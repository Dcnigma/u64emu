// iMemory.cpp - N64 Memory Abstraction for libnx (Switch, post-2021)
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <switch.h>
#include "iMemory.h"
#include "iCPU.h"
#include "iATA.h"
#include "iRom.h"

using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;

#define N_SEGMENTS 23

// Memory sizes for each segment (adjust as needed)
static const size_t MemSize[N_SEGMENTS] = {
    8*1024*1024 + 512*1024, // RAM
    0x100, 0x100, 0x7c0, 0x40, 0x40,
    1024, 0x20, 0x20, 0x10, 0x10, 0x10, 0x38,
    0x1000, 0x34, 0x34,
    0x20, 0x1c, 1024, 0x1000,
    0x8000, 0x8000, 4*1024*1024 + 4096 // DSP R/PM/DMEM
};

// Memory pointers
static BYTE* iMemAddr[N_SEGMENTS] = {0};
N64Mem* m = nullptr;

// Safe allocation
static BYTE* SafeMalloc(size_t size) {
    BYTE* ptr = (BYTE*)malloc(size);
    if (!ptr) {
        printf("Fatal: Out of memory!\n");
        abort();
    }
    return ptr;
}

static void SafeFree(void* ptr) {
    if (ptr) free(ptr);
}

// Initialize memory
void iMemInit() {
    for (int i = 0; i < N_SEGMENTS; ++i)
        iMemAddr[i] = SafeMalloc(MemSize[i]);

    m = new N64Mem();

    m->rdRam   = iMemAddr[0];
    m->spDmem  = iMemAddr[1];
    m->spImem  = iMemAddr[2];
    m->piRom   = iMemAddr[3];
    m->piRam   = iMemAddr[4];
    m->piRamW  = iMemAddr[5];
    m->rdReg   = iMemAddr[6];
    m->spReg   = iMemAddr[7];
    m->dpcReg  = iMemAddr[8];
    m->dpsReg  = iMemAddr[9];
    m->miReg   = iMemAddr[10];
    m->miRegW  = iMemAddr[11];
    m->viReg   = iMemAddr[12];
    m->aiReg   = iMemAddr[13];
    m->piReg   = iMemAddr[14];
    m->piRegW  = iMemAddr[15];
    m->riReg   = iMemAddr[16];
    m->siReg   = iMemAddr[17];
    m->NullMem = iMemAddr[18];
    m->atReg   = iMemAddr[19];
    m->dspPMem = iMemAddr[20];
    m->dspDMem = iMemAddr[21];
    m->dspRMem = iMemAddr[22];
}

// Clear memory
void iMemClear() {
    memset(m->rdRam, 0, MemSize[0]);
    memset(m->NullMem, 0, MemSize[18]);
    memset(m->dspPMem, 0, MemSize[20]);
    memset(m->dspDMem, 0, MemSize[21]);
    memset(m->dspRMem, 0, MemSize[22]);
}

// Free memory
void iMemDestruct() {
    for (int i = 0; i < N_SEGMENTS; ++i)
        SafeFree(iMemAddr[i]);
    delete m;
    m = nullptr;
}

// Load ROM into PI ROM
bool iMemLoadROM(const char* path) {
    std::ifstream romFile(path, std::ios::binary | std::ios::ate);
    if (!romFile.is_open()) {
        printf("Failed to open ROM file: %s\n", path);
        return false;
    }

    std::streamsize size = romFile.tellg();
    romFile.seekg(0, std::ios::beg);

    if (size > static_cast<std::streamsize>(MemSize[3])) {
        printf("ROM too large for PI ROM memory segment.\n");
        return false;
    }

    romFile.read(reinterpret_cast<char*>(m->piRom), size);
    printf("Loaded ROM %s (%ld bytes)\n", path, size);
    return true;
}

// -------- CPU Memory Access --------
BYTE iMemReadByte(DWORD addr) {
    if (addr < MemSize[0])
        return m->rdRam[addr];
    return m->NullMem[0];
}

void iMemWriteByte(DWORD addr, BYTE val) {
    if (addr < MemSize[0])
        m->rdRam[addr] = val;
}

WORD iMemReadWord(DWORD addr) {
    return (iMemReadByte(addr) << 8) | iMemReadByte(addr + 1);
}

DWORD iMemReadDWord(DWORD addr) {
    return (iMemReadWord(addr) << 16) | iMemReadWord(addr + 2);
}

void iMemWriteWord(DWORD addr, WORD val) {
    iMemWriteByte(addr, val >> 8);
    iMemWriteByte(addr + 1, val & 0xFF);
}

void iMemWriteDWord(DWORD addr, DWORD val) {
    iMemWriteWord(addr, val >> 16);
    iMemWriteWord(addr + 2, val & 0xFFFF);
}

// -------- DSP Memory Access --------
BYTE dspReadByte(DWORD addr, bool isDMem) {
    if (isDMem)
        return m->dspDMem[addr % MemSize[21]];
    return m->dspPMem[addr % MemSize[20]];
}

void dspWriteByte(DWORD addr, BYTE val, bool isDMem) {
    if (isDMem)
        m->dspDMem[addr % MemSize[21]] = val;
    else
        m->dspPMem[addr % MemSize[20]] = val;
}

BYTE dspReadRByte(DWORD addr) {
    return m->dspRMem[addr % MemSize[22]];
}

void dspWriteRByte(DWORD addr, BYTE val) {
    m->dspRMem[addr % MemSize[22]] = val;
}

// -------- ATA Access --------
BYTE ataReadRegister(BYTE reg) {
    return m->atReg[reg % MemSize[19]];
}

void ataWriteRegister(BYTE reg, BYTE val) {
    m->atReg[reg % MemSize[19]] = val;
}

// -------- SP Registers --------
BYTE spReadReg(BYTE reg) {
    return m->spReg[reg % MemSize[7]];
}

void spWriteReg(BYTE reg, BYTE val) {
    m->spReg[reg % MemSize[7]] = val;
}

// -------- PI Registers --------
BYTE piReadReg(BYTE reg) {
    return m->piReg[reg % MemSize[14]];
}

void piWriteReg(BYTE reg, BYTE val) {
    m->piReg[reg % MemSize[14]] = val;
}

// -------- AI Registers --------
BYTE aiReadReg(BYTE reg) {
    return m->aiReg[reg % MemSize[13]];
}

void aiWriteReg(BYTE reg, BYTE val) {
    m->aiReg[reg % MemSize[13]] = val;
}

// -------- VI Registers --------
BYTE viReadReg(BYTE reg) {
    return m->viReg[reg % MemSize[12]];
}

void viWriteReg(BYTE reg, BYTE val) {
    m->viReg[reg % MemSize[12]] = val;
}

// -------- RI Registers --------
BYTE riReadReg(BYTE reg) {
    return m->riReg[reg % MemSize[16]];
}

void riWriteReg(BYTE reg, BYTE val) {
    m->riReg[reg % MemSize[16]] = val;
}

// -------- SI Registers --------
BYTE siReadReg(BYTE reg) {
    return m->siReg[reg % MemSize[17]];
}

void siWriteReg(BYTE reg, BYTE val) {
    m->siReg[reg % MemSize[17]] = val;
}

// -------- DMA Utility (simplified) --------
void dmaToRam(const BYTE* src, DWORD dstAddr, size_t size) {
    if (dstAddr + size <= MemSize[0])
        memcpy(&m->rdRam[dstAddr], src, size);
}

void dmaFromRam(BYTE* dst, DWORD srcAddr, size_t size) {
    if (srcAddr + size <= MemSize[0])
        memcpy(dst, &m->rdRam[srcAddr], size);
}

// -------- Debug Utility --------
void iMemDumpSegment(int seg) {
    if (seg < 0 || seg >= N_SEGMENTS) return;
    printf("Memory segment %d (%zu bytes) first 16 bytes:\n", seg, MemSize[seg]);
    for (int i = 0; i < 16 && i < MemSize[seg]; ++i)
        printf("%02X ", iMemAddr[seg][i]);
    printf("\n");
}
