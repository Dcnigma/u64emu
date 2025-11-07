#include <switch.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "iCPU.h"
#include "iMemory.h"
#include "iRom.h"
#include "iGeneralOps.h"
#include "iBranchOps.h"
#include "iFPOps.h"
#include "iMemoryOps.h"
#include "iIns.h"
#include "hleMain.h"
#include "hleDSP.h"

// --- Emulated CPU/DSP state ---
static RS4300iReg* r = nullptr;
static u32 iCpuNextVSYNC = 0;
static bool iCpuResetVSYNC = false;
static u32 iCpuVSYNCAccum = 0;

// --- Thread management ---
static Thread cpuThread;
static Thread dspThread;
static bool cpuRunning = true;
static bool dspRunning = true;

// --- Timing ---
static u64 hleDSPPingTime = 0;

// Emulated registers / app context placeholders
extern "C" {
    extern void dynaInit();
    extern void dynaDestroy();
    extern bool dynaExecute(u32 pc);
    extern void hleISR();
    extern void hleISR2();
}

// ------------------ CPU Construction ------------------

void iCpuConstruct() {
    r = (RS4300iReg*)malloc(sizeof(RS4300iReg));
    memset(r, 0, sizeof(RS4300iReg));

    dynaInit();
    // Placeholder for dynamic recompiler setup
}

// ------------------ CPU Destruction ------------------

void iCpuDestruct() {
    dynaDestroy();
    if (r) free(r);
    r = nullptr;
    cpuRunning = false;
}

// ------------------ CPU Reset ------------------

void iCpuReset() {
    memset(r, 0, sizeof(RS4300iReg));
    r->PC = 0xA4000040;
    for (int i = 0; i < 64; i++) r->GPR[i] = 0;

    r->GPR[29 * 2] = 0xA4001FF0;
    r->Hi = 0;
    r->Lo = 0;
    r->Llbit = 0;
    r->Delay = NO_DELAY;

    r->CurRoundMode = 0x027f;
    r->RoundMode = 0x027f;
    r->TruncMode = 0x0e7f;
    r->CeilMode = 0x0a7f;
    r->FloorMode = 0x067f;
    r->CPR0[2 * STATUS] = 0x50400004;
    r->CPR0[2 * RANDOM] = 0x0000002f;
    r->CPR0[2 * CONFIG] = 0x00066463;
    r->CCR1[0] = 0x00000511;

    r->ICount = 1;
    r->NextIntCount = 6250000;
    r->CompareCount = 0;
    r->VTraceCount = 6250000;
}

// ------------------ DSP Thread ------------------

void dspThreadFunc(void*) {
    svcSleepThread(1'000'000'000); // 1s delay before start

    while (dspRunning) {
        hleDSPMain();
        hleDSPMain2();

        if (hleDSPPingTime == 0)
            hleDSPPingTime = armGetSystemTick() + armTicksPerMs() * 7;

        // Placeholder for audio scheduling / buffer update
        svcSleepThread(7'000'000); // 7ms
        hleDSPMain3();
    }
}

// ------------------ CPU Thread ------------------

void cpuThreadFunc(void*) {
    svcSleepThread(1'000'000'000); // 1s startup delay

    while (cpuRunning) {
        // --- Simplified main loop ---
        if ((r->PC & 0xFF000000) == 0x88000000) {
            u32 op = *(u32*)&m->rdRam[r->PC & MEM_MASK];
            iOpCode = op;
        } else {
            u32 op = *(u32*)&rom->Image[r->PC & 0x7FFFF];
            iOpCode = op;
        }

        r->PC += 4;
        iMain[iOpCode >> 26]();
        r->GPR[0] = 0;
        r->GPR[1] = 0;

        switch (r->Delay) {
            case DO_DELAY:
                r->Delay = EXEC_DELAY;
                break;
            case EXEC_DELAY:
                r->Delay = NO_DELAY;
                r->PC = r->PCDelay;
                break;
        }

        iCpuVSYNC();
    }
}

// ------------------ VSYNC / Timing ------------------

void iCpuVSYNC() {
    u64 now = armGetSystemTick();
    if (iCpuNextVSYNC == 0)
        iCpuNextVSYNC = now + armTicksPerMs() * 16;

    s64 diff = (s64)iCpuNextVSYNC - (s64)now;
    if (diff > 0)
        svcSleepThread(diff);

    iCpuNextVSYNC += armTicksPerMs() * 16;
    r->ICount = r->NextIntCount;
    r->VTraceCount += 833333;
    r->NextIntCount = r->VTraceCount;
    hleISR();
}

// ------------------ Thread Control ------------------

void iCpuStartThreads() {
    threadCreate(&cpuThread, cpuThreadFunc, nullptr, 0x4000, 0x2B, -2);
    threadStart(&cpuThread);

    threadCreate(&dspThread, dspThreadFunc, nullptr, 0x4000, 0x2C, -2);
    threadStart(&dspThread);
}

void iCpuStopThreads() {
    cpuRunning = false;
    dspRunning = false;
    threadWaitForExit(&cpuThread);
    threadWaitForExit(&dspThread);
    threadClose(&cpuThread);
    threadClose(&dspThread);
}
