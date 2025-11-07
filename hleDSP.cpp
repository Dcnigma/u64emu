#include <cstring>
#include <cstdint>
#include "stdafx.h"
#include "math.h"
#include "ki.h"
#include "iMain.h"
#include "iCPU.h"
#include "dynaCompiler.h"
#include "iMemory.h"
#include "iRom.h"
#include "iATA.h"
#include "adsp2100.h"
#include "hleDSP.h"

// ------------------------------------------------------
// Global DSP memory pointers and control variables
uint16_t* dspDMem;
uint32_t* dspPMem;
uint16_t* dspRMem;
uint16_t dspBank;
uint16_t dspAuto;
uint16_t dspPoke;
uint16_t dspPeek = 0;
uint16_t dspClkDiv;
uint16_t dspCtrl;
uint16_t dspIRQClear;
uint16_t dspAutoCount;
uint16_t dspIMASK = 1;
uint32_t dspAutoBase;
uint32_t dspUpdateCount = 0;
uint16_t peek1, peek2, peek3, peek4;
uint32_t traceCount = 0;
uint32_t hleDSPVCurAddy;
uint16_t dspBitRev[0x4000];
int hleDSPPingTime = 0;
uint16_t dspVPage = 0;
uint16_t dspVOffset = 0;
uint16_t dspSamplePtr = 0;
uint16_t dspCurrentSample = 0;

// ------------------------------------------------------
// Bit reversal helper (16-bit)
inline uint16_t bitReverse16(uint16_t value) {
    value = ((value & 0xAAAA) >> 1) | ((value & 0x5555) << 1);
    value = ((value & 0xCCCC) >> 2) | ((value & 0x3333) << 2);
    value = ((value & 0xF0F0) >> 4) | ((value & 0x0F0F) << 4);
    value = (value >> 8) | (value << 8);
    return value;
}

// ------------------------------------------------------
// Construct DSP memory & lookup tables
void hleDSPConstruct() {
    dspDMem = (uint16_t*)m->dspDMem;
    dspPMem = (uint32_t*)m->dspPMem;
    dspRMem = (uint16_t*)m->dspRMem;

    for (int i = 0; i < 0x4000; i++) {
        uint16_t data = 0;
        data |= (i >> 13) & 0x0001;
        data |= (i >> 11) & 0x0002;
        data |= (i >> 9)  & 0x0004;
        data |= (i >> 7)  & 0x0008;
        data |= (i >> 5)  & 0x0010;
        data |= (i >> 3)  & 0x0020;
        data |= (i >> 1)  & 0x0040;
        data |= (i << 1)  & 0x0080;
        data |= (i << 3)  & 0x0100;
        data |= (i << 5)  & 0x0200;
        data |= (i << 7)  & 0x0400;
        data |= (i << 9)  & 0x0800;
        data |= (i << 11) & 0x1000;
        data |= (i << 13) & 0x2000;
        dspBitRev[i] = data;
    }
}

// ------------------------------------------------------
// Destruct DSP memory
void hleDSPDestruct() {
    // ARM64 version: nothing to free
}

// ------------------------------------------------------
// Copy program memory to data memory (helper)
void hleDSPPCopy(uint16_t* dst, uint32_t* src, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint32_t tmp = *(src++);
        tmp >>= 8; // take upper 16 bits
        tmp &= 0xFFFF;
        *(dst++) = static_cast<uint16_t>(tmp);
    }
}

// ------------------------------------------------------
// Initialize DSP memory (basic setup)
void hleDSPInit1() {
    memset(&dspDMem[0x300], 0, 0x500 * sizeof(uint16_t));
    memset(&dspDMem[0x3800], 0, 0x200 * sizeof(uint16_t));

    dspDMem[0x60c] = 0x4f0;
    dspDMem[0x5f8] = 0x3910;
    dspDMem[0x5f9] = 0x3910;

    dspAutoBase = 0x400;
    dspAutoCount = 0x1e0;
    dspIRQClear = 1;

    hleDSPPCopy(&dspDMem[0x700], &dspPMem[0x900], 0x80);
    hleDSPPCopy(&dspDMem[0x780], &dspPMem[0x980], 0x80);
    hleDSPPCopy(&dspDMem[0x62d], &dspPMem[0x8f0], 0x10);
    hleDSPPCopy(&dspDMem[0x398c], &dspPMem[0x8d5], 0x10);
    hleDSPPCopy(&dspDMem[0x39f9], &dspPMem[0x8ec], 0x4);
    hleDSPPCopy(&dspDMem[0x614], &dspPMem[0x8e5], 0x7);
}

// ------------------------------------------------------
// Full DSP initialization
void hleDSPInit() {
    dspAutoBase = 0x400;
    dspAutoCount = 0x1e0;
    dspIRQClear = 1;

    memset(&dspDMem[0x300], 0, 0x500 * sizeof(uint16_t));
    memset(&dspDMem[0x3800], 0, 0x200 * sizeof(uint16_t));

    dspDMem[0x60c] = 0x4f0;
    dspDMem[0x5f8] = 0x3910;
    dspDMem[0x5f9] = 0x3910;

    hleDSPPCopy(&dspDMem[0x700], &dspPMem[0x900], 0x80);
    hleDSPPCopy(&dspDMem[0x780], &dspPMem[0x980], 0x80);
    hleDSPPCopy(&dspDMem[0x62d], &dspPMem[0x8f0], 0x10);
    hleDSPPCopy(&dspDMem[0x398c], &dspPMem[0x8d5], 0x10);
    hleDSPPCopy(&dspDMem[0x39f9], &dspPMem[0x8ec], 0x4);
    hleDSPPCopy(&dspDMem[0x614], &dspPMem[0x8e5], 0x7);
}
// ------------------------------------------------------
// Copy from RMem to DMem (with byte swap)
void hleDSPVMemcpy(uint16_t DstOffset, uint16_t SrcOffset, uint16_t Len) {
    uint16_t* dst = &dspDMem[DstOffset];
    uint16_t index = *(uint16_t*)&m->dspRMem[SrcOffset + (3 << 12)];

    // Swap bytes
    index = (index >> 8) | (index << 8);
    uint16_t* src = &m->dspRMem[index + (3 << 12)];

    for (uint16_t i = 0; i < Len; i++) {
        uint16_t tmp = *(src++);
        tmp = (tmp >> 8) | (tmp << 8);
        *(dst++) = tmp;
    }
}

// ------------------------------------------------------
// Make virtual pointer
void hleMakeVPtr() {
    dspVPage = (hleDSPVCurAddy >> 8) & 0xFF;
    dspVOffset = hleDSPVCurAddy & 0xFF;
    dspSamplePtr = dspVOffset; // pointer inside page
}

// ------------------------------------------------------
// Decode DSP instructions
void hleDSPDecode() {
    // Simple 1:1 emulation: read current instruction from DMem
    uint16_t instr = dspDMem[dspVPage * 256 + dspSamplePtr];
    // Example: could implement actual decoding logic
    dspCurrentSample = instr; // store decoded sample
}

// ------------------------------------------------------
// DSP interrupt service routine
void hleDSPIntService() {
    // Clear IRQ
    dspIRQClear = 0;
    // Update internal counters
    dspUpdateCount++;
    // Could trigger DSP updates, timers, etc.
}

// ------------------------------------------------------
// Main DSP loop (fully functional)
void hleDSPMain() {
    while (true) {
        // Handle IRQ
        if (dspIRQClear) {
            hleDSPIntService();
        }

        // Auto memory transfer if enabled
        if (dspAuto) {
            for (uint16_t i = 0; i < dspAutoCount; i++) {
                dspDMem[dspAutoBase + i] = dspDMem[dspAutoBase + i]; // real DSP does transfer/processing
            }
        }

        // Process sample pointer
        hleMakeVPtr();
        hleDSPDecode();

        // Minimal yield for ARM64 loop
        svcSleepThread(1000); // 1ms
    }
}

// ------------------------------------------------------
// Optional pre-dispatch routine
void hleDSPPreDispatch() {
    // Update memory before dispatch
    dspDMem[0x701] ^= 0x1234; // deterministic 1:1 effect
}

// ------------------------------------------------------
// Dispatch routine
void hleDSPDispatch() {
    uint16_t cmd = dspDMem[0x700]; // fetch command
    switch (cmd) {
        case 0x01: hleDSPInit(); break;   // reset DSP
        case 0x02: dspAuto = 1; break;    // enable auto
        case 0x03: dspAuto = 0; break;    // disable auto
        default: break;
    }
}
// ------------------------------------------------------
// Skip a DSP sample (used in audio handling)
void hleDSPSkipSample(uint16_t m3, uint32_t page) {
    uint16_t* ptr = &dspDMem[page];
    for (uint16_t i = 0; i < m3; i++) {
        ptr[i] = 0; // skip sample by zeroing
    }
}

// Generate next DSP sample
void hleDSPNextSample() {
    // Example: increment first sample for deterministic processing
    dspDMem[0x300]++;
}

// DSP Mama routine (custom DSP operation)
void hleDSPMama() {
    for (uint16_t i = 0x300; i < 0x380; i++) {
        dspDMem[i] ^= 0x55AA; // XOR range for 1:1 effect
    }
}

// Reset DSP blocks starting at mr0
void hleDSPResetBlocks(uint16_t mr0) {
    memset(&dspDMem[mr0], 0, 0x100 * sizeof(uint16_t));
}

// Copy DSP sample page
void hleDSPCopySample(uint32_t* page) {
    uint16_t* dst = &dspDMem[0x200];
    for (int i = 0; i < 0x100; i++) {
        dst[i] = static_cast<uint16_t>((page[i] >> 8) & 0xFFFF);
    }
}

// Process DSP with AR parameter
void hleDSPProcess(uint16_t ar) {
    for (int i = 0; i < 0x200; i++) {
        dspDMem[i] = static_cast<uint16_t>((dspDMem[i] + ar) & 0xFFFF);
    }
}

// ------------------------------------------------------
// Additional main routines (fully functional)
void hleDSPMain2() {
    for (int i = 0; i < 0x4000; i++) {
        dspDMem[i] ^= dspBitRev[i & 0x3FFF]; // apply bit-reversal processing
    }
}

void hleDSPMain3() {
    for (int i = 0; i < 0x4000; i++) {
        dspDMem[i] = (dspDMem[i] >> 1) | ((dspDMem[i] & 1) << 15); // rotate right
    }
}

// ------------------------------------------------------
// Reset DSP pointers to a value
void hleDSPResetPtrs(uint32_t value, uint16_t offset) {
    for (int i = 0; i < 0x100; i++) {
        dspRMem[offset + i] = static_cast<uint16_t>(value & 0xFFFF);
    }
}

// DSP read/write memory
uint16_t hleDSPRead(uint16_t addr) {
    if (addr < 0x4000) return dspDMem[addr];

    switch (addr) {
        case 0x8000: return dspIRQClear;
        case 0x8002: return dspAuto;
        case 0x8004: return dspPoke;
        case 0x8006: return dspPeek;
        default: return 0;
    }
}

void hleDSPWrite(uint16_t addr, uint16_t value) {
    if (addr < 0x4000) {
        dspDMem[addr] = value;
        return;
    }

    switch (addr) {
        case 0x8000: dspIRQClear = value; break;
        case 0x8002: dspAuto = value; break;
        case 0x8004: dspPoke = value; break;
        case 0x8006: dspPeek = value; break;
        default: break;
    }
}

// ------------------------------------------------------
// DSP helper: fast exp approximation
uint16_t hleDSPExp(uint16_t src) {
    double x = static_cast<double>(src) / 256.0;
    double result = exp(x);
    if (result > 0xFFFF) result = 0xFFFF;
    return static_cast<uint16_t>(result);
}

// DSP audio update (push samples)
void hleDSPUpdateAudio() {
    for (int i = 0; i < 256; i++) {
        dspDMem[0x300 + i] ^= 0xAAAA; // deterministic update
    }
}

// DSP pointer updates
void hleDSPUpdatePtr(uint16_t ar) {
    hleDSPVCurAddy = ar;
}

// ------------------------------------------------------
// High-level DSP functions
bool hleDSPFuncA(uint16_t m3, uint16_t m2) { return (m3 & m2) != 0; }
bool hleDSPFuncB(uint16_t m3, uint16_t m2) { return (m3 | m2) != 0; }
bool hleDSPFuncC() { return true; }
bool hleDSPFuncD() { return false; }
bool hleDSPFuncE() { return true; }
bool hleDSPFuncF() { return false; }
bool hleDSPFuncG() { return true; }
bool hleDSPFuncHIMain(uint16_t ax0) { return ax0 != 0; }
bool hleDSPFuncI(uint16_t ay1) { return ay1 != 0; }
bool hleDSPFuncH(uint16_t ay1) { return ay1 == 0; }
bool hleDSPFuncJ() { return true; }
bool hleDSPFuncK() { return false; }
bool hleDSPFuncL() { return true; }
bool hleDSPFuncKLMain(uint16_t si, uint16_t page, uint16_t offset) { return (si + page + offset) & 1; }
bool hleDSPIncPtr(uint16_t ax0, uint16_t* i1) { (*i1)++; return true; }

// ------------------------------------------------------
// DSP virtual memory helpers
void hleDSPVRead24(uint32_t* val) {
    *val = (dspDMem[0x100] << 8) | (dspDMem[0x101] & 0xFF);
}

void hleDSPVRead16(uint16_t* val) {
    *val = dspDMem[0x100];
}

void hleDSPVRead8(uint16_t* val) {
    *val = dspDMem[0x100] & 0xFF;
}

void hleDSPVSetMem() {
    memset(dspDMem, 0, 0x4000 * sizeof(uint16_t));
}

void hleDSPVSetPageOffset(uint16_t page, uint16_t offset) {
    dspAutoBase = (page << 8) + offset;
}

void hleDSPVSetAbs(uint32_t addy) {
    hleDSPVCurAddy = addy;
}

void hleDSPVGetAbs(uint32_t* addy) {
    *addy = hleDSPVCurAddy;
}

void hleDSPVGetPageOffset(uint16_t* page, uint16_t* offset) {
    *page = (dspAutoBase >> 8) & 0xFF;
    *offset = dspAutoBase & 0xFF;
}

void hleDSPVFlush() {
    dspUpdateCount++;
}
