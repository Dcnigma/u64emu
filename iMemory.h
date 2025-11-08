#ifndef IMEM_H
#define IMEM_H

#include <switch.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include "N64Mem.h"   // include the full struct now

using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;
using QWORD = uint64_t;

// Global memory pointer
extern N64Mem* m;

// Memory control variables
extern WORD iMemToDo;
extern WORD iMemTLBActive;

// Last read/write addresses
extern DWORD iMemWriteByteAddress;
extern DWORD iMemWriteWordAddress;
extern DWORD iMemWriteDWordAddress;
extern DWORD iMemWriteQWordAddress;
extern DWORD iMemReadByteAddress;
extern DWORD iMemReadWordAddress;
extern DWORD iMemReadDWordAddress;
extern DWORD iMemReadQWordAddress;

// DSP memory
extern unsigned char* DSPPMem;
extern unsigned char* DSPDMem;
extern unsigned char* DSPRMem;

// Construct/destruct/init/clear memory
void iMemInit();
void iMemConstruct();
void iMemClear();
void iMemDestruct();
void iMemFinalCheck();
void iMemCopyBootCode();

// Memory access
BYTE* iMemPhysReadAddr(DWORD vAddr);
BYTE* iMemPhysWriteAddr(DWORD vAddr);

BYTE iMemReadByte(DWORD addr);
WORD iMemReadWord(DWORD addr);
DWORD iMemReadDWord(DWORD addr);
QWORD iMemReadQWord(DWORD addr);

void iMemWriteByte(BYTE val, DWORD addr);
void iMemWriteWord(DWORD addr, WORD val);
void iMemWriteDWord(DWORD addr, DWORD val);
void iMemWriteQWord(QWORD val, DWORD addr);
void iMemWriteByte(DWORD addr, BYTE val);

// DSP access
BYTE dspReadByte(DWORD addr, bool isDMem);
int16_t dspReadWord(DWORD addr);
DWORD dspReadDWord(DWORD addr);

void dspWriteByte(DWORD addr, BYTE val, bool isDMem);
void dspWriteWord(DWORD addr, WORD val);
void dspWriteDWord(DWORD addr, DWORD val);

// Register update hooks
void iMemUpdateAIReg();
void iMemUpdateVIReg();
void iMemUpdatePIReg();
void iMemUpdateMIReg();
void iMemUpdateSPReg();
void iMemUpdateDPReg();

// Display/boot helpers
void iMemDoDisplayRefresh();

// Save/load helpers
void iMemSave(std::ofstream& out);
void iMemLoad(std::ifstream& in);
void iMemLoadShort(std::ifstream& in);

#endif // IMEM_H
