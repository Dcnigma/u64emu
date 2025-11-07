#include "iMain.h"
#include "iCPU.h"       // Core 4600 emulation routines
#include "iMemory.h"    // Memory emulation routines
#include "iRom.h"       // ROM (cart) emulation routines
#include "dynaCompiler.h"
#include "adsp2100.h"
#include "hleDSP.h"
#include <switch.h>
#include <thread>
#include <fstream>
#include <chrono>

volatile uint16_t DspTask;
volatile uint16_t NewTask;
extern bool dump;

RS4300iReg *r;       // Registers
N64Mem *m;           // Memory
N64RomStruct *rom;

uint32_t iOpCode;        // Current opcode
uint32_t iNextOpCode;    // Next opcode
uint32_t iPC;            // Current PC copy
uint16_t iFPUMode = 0x027f; // Rounds to nearest

char iRegName[32][3] = {
    "R0","At","V0","V1","A0","A1","A2","A3",
    "T0","T1","T2","T3","T4","T5","T6","T7",
    "S0","S1","S2","S3","S4","S5","S6","S7",
    "T8","T9","K0","K1","GP","SP","S8","RA"
};

// Modern thread handles
std::thread iCpuThread;
std::thread iDspThread;

// 'Reset' the main CPU and setup for N64 emulation
void iMainConstruct(const char *filename)
{
    iRomConstruct();
    iMemConstruct();
    iCpuConstruct();
    iDspThread = std::thread(); // Placeholder; will start later
    hleDSPConstruct();
}

void iMainDestruct()
{
    hleDSPDestruct();
    iMemDestruct();
    iRomDestruct();
    iCpuDestruct();
    adsp2100_exit();
}

void iMainReset()
{
    iCpuReset();
    dspBank = 1;
    iMainResetDSP();
}

void iMainResetDSP()
{
    uint32_t *dst, *src, tmp;
    src = reinterpret_cast<uint32_t *>(&m->dspRMem[dspBank << 12]);
    dst = reinterpret_cast<uint32_t *>(m->dspPMem);
    uint32_t size = *src;
    size >>= 24;
    size++;
    size <<= 3;
    while(size)
    {
        tmp = *(src++);
        tmp = ((tmp & 0xff0000) >> 16) | ((tmp & 0xff) << 16) | (tmp & 0xff00);
        *(dst++) = tmp;
        size--;
    }
    adsp2100_reset(nullptr);
}

void iMainStartCPU()
{
    NewTask = BOOT_STAGE0;

    std::ifstream infile;
    if(gRomSet == KI2)
        infile.open("ki2.dat", std::ios::binary | std::ios::in);
    else if(gRomSet == KI1)
        infile.open("ki.dat", std::ios::binary | std::ios::in);

    if(infile.is_open())
    {
        infile.read(reinterpret_cast<char *>(m->rdRam), 0x100000);
        infile.read(reinterpret_cast<char *>(r), sizeof(RS4300iReg));
        infile.close();

        NewTask = NORMAL_GAME;
        dump = true;
    }

    adsp2100_set_pc(0);
    DspTask = NORMAL_GAME;

    // Launch CPU and DSP threads using std::thread
    iCpuThread = std::thread([](){ iCpuThreadProc(); });
    iDspThread = std::thread([](){ iDspThreadProc(); });
}

void iMainStopCPU()
{
    NewTask = EXIT_EMU;
    DspTask = EXIT_EMU;

    auto waitLimit = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);

    while(NewTask && std::chrono::steady_clock::now() < waitLimit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if(iCpuThread.joinable())
        iCpuThread.detach(); // Or implement proper shutdown in thread proc

    if(iDspThread.joinable())
        iDspThread.detach(); // Or implement proper shutdown in thread proc
}
