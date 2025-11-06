#include "stdafx.h"
#include "math.h"
#include "ki.h"
#include <malloc.h>      // for memalign
#include <string.h>      // for memset
#include "iMain.h"
#include "iCPU.h"
#include "iMemory.h"
#include "iRom.h"
#include "dynaCompiler.h"
#include "adsp2100.h"
#include "hleDSP.h"
#include "global.h"

#define STACKSIZE (8 * 1024)

// Globals
volatile WORD DspTask;
volatile WORD NewTask;
extern bool dump;

RS4300iReg *r = nullptr;   // CPU registers
N64Mem *m = nullptr;       // N64 memory
N64RomStruct *rom = nullptr;

DWORD iOpCode;       // current opcode
DWORD iNextOpCode;   // next opcode
DWORD iPC;           // program counter copy
WORD iFPUMode = 0x027f;  // FPU rounding mode

static u64 s_startTicks;

// Thread handles and stacks
static Thread cpu_thread;
static Thread dsp_thread;
static void* cpu_stack = nullptr;
static void* dsp_stack = nullptr;

// Register names
char iRegName[32][3] = {
    "R0","At","V0","V1","A0","A1","A2","A3",
    "T0","T1","T2","T3","T4","T5","T6","T7",
    "S0","S1","S2","S3","S4","S5","S6","S7",
    "T8","T9","K0","K1","GP","SP","S8","RA"
};

// --- Helper: get elapsed time ---
float getTime() {
    u64 elapsed = armGetSystemTick() - s_startTicks;
    return (elapsed * 625 / 12) / 1000000000.0;
}



// --- Initialize main emulator ---
void iMainConstruct(char *filename)
{
    s_startTicks = armGetSystemTick();

    // Allocate CPU registers
    r = (RS4300iReg*)malloc(sizeof(RS4300iReg));
    if (!r) {
        svcOutputDebugString("Failed to allocate CPU registers!\n", 36);
        return;
    }
    memset(r, 0, sizeof(RS4300iReg));

    // Allocate N64 memory
    m = (N64Mem*)malloc(sizeof(N64Mem));
    if (!m) {
        svcOutputDebugString("Failed to allocate N64 memory!\n", 33);
        free(r);
        r = nullptr;
        return;
    }
    memset(m, 0, sizeof(N64Mem));

    // Allocate ROM struct
    rom = (N64RomStruct*)malloc(sizeof(N64RomStruct));
    if (!rom) {
        svcOutputDebugString("Failed to allocate ROM struct!\n", 34);
        free(r); r = nullptr;
        free(m); m = nullptr;
        return;
    }
    memset(rom, 0, sizeof(N64RomStruct));

    iRomConstruct();
    iMemConstruct();
    iCpuConstruct();
    iDspThreadId = NULL;
    hleDSPConstruct();
}

// --- Cleanup ---
void iMainDestruct()
{
    hleDSPDestruct();
    iMemDestruct();
    iRomDestruct();
    iCpuDestruct();
    adsp2100_exit();

    if (r) { free(r); r = nullptr; }
    if (m) { free(m); m = nullptr; }
    if (rom) { free(rom); rom = nullptr; }
}

// --- Reset CPU and DSP ---
void iMainReset()
{
    iCpuReset();
    dspBank = 1;
    iMainResetDSP();
}

void iMainResetDSP()
{
    DWORD *dst, *src, tmp;
    src = (DWORD*)&m->dspRMem[dspBank << 12];
    dst = (DWORD*)m->dspPMem;

    DWORD size = *src;
    size >>= 24;
    size++;
    size <<= 3;

    while (size) {
        tmp = *(src++);
        tmp = (tmp & 0xff0000) >> 16 | (tmp & 0xff) << 16 | (tmp & 0xff00);
        *(dst++) = tmp;
        size--;
    }
    adsp2100_reset(NULL);
}

// --- Start CPU and DSP threads ---
void iMainStartCPU()
{
    int dwThrdParam = 0;
    NewTask = BOOT_STAGE0;
    DspTask = BOOT_STAGE0;

    const char* romFilename = nullptr;

    if (gRomSet == KI2)
        romFilename = "ki2.dat";
    else if (gRomSet == KI1)
        romFilename = "ki.dat";

    if (!romFilename) {
        svcOutputDebugString("No ROM selected!\n", 20);
        return;
    }

    FILE* tmpf = fopen(romFilename, "rb");
    if (!tmpf) {
        svcOutputDebugString("ROM file missing!\n", 23);
        return;
    }

    // Load ROM into RAM safely
    size_t bytesRead = fread(m->rdRam, 1, 0x100000, tmpf);
    fclose(tmpf);
    if (bytesRead != 0x100000) {
        svcOutputDebugString("Failed to read full RAM from ROM!\n", 39);
        return;
    }

    memset(r, 0, sizeof(RS4300iReg));  // Reset CPU registers

    dump = true;
    NewTask = NORMAL_GAME;
    DspTask = NORMAL_GAME;

    adsp2100_set_pc(0);

    // Allocate aligned stacks
    cpu_stack = memalign(0x1000, STACKSIZE);
    dsp_stack = memalign(0x1000, STACKSIZE);
    if (!cpu_stack || !dsp_stack) {
        svcOutputDebugString("Failed to allocate thread stacks!\n", 38);
        if (cpu_stack) free(cpu_stack);
        if (dsp_stack) free(dsp_stack);
        return;
    }

    Result rc;

    rc = threadCreate(&cpu_thread, (ThreadFunc)iCpuThreadProc, &dwThrdParam,
                      cpu_stack, STACKSIZE, 0x2C, 1);
    if (R_FAILED(rc)) {
        svcOutputDebugString("CPU thread creation failed!\n", 30);
        return;
    }
    threadStart(&cpu_thread);

    rc = threadCreate(&dsp_thread, (ThreadFunc)iDspThreadProc, &dwThrdParam,
                      dsp_stack, STACKSIZE, 0x2F, 2);
    if (R_FAILED(rc)) {
        svcOutputDebugString("DSP thread creation failed!\n", 30);
        return;
    }
    threadStart(&dsp_thread);
}

// --- Stop CPU and DSP threads ---
void iMainStopCPU()
{
    NewTask = EXIT_EMU;
    DspTask = EXIT_EMU;

    // Wait for threads to exit
    DWORD WaitLimit = getTime() + 2000; // 2 seconds max
    while ((NewTask || DspTask) && (getTime() < WaitLimit)) {
        svcSleepThread(200000000); // 200 ms
    }

    threadWaitForExit(&cpu_thread);
    threadWaitForExit(&dsp_thread);

    if (cpu_stack) { free(cpu_stack); cpu_stack = nullptr; }
    if (dsp_stack) { free(dsp_stack); dsp_stack = nullptr; }

    svcOutputDebugString("CPU and DSP threads stopped.\n", 32);
}
