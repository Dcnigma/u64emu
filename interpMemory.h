#pragma once
#include <switch.h>
#include <stdint.h>

// Standardized types
typedef uint8_t  BYTE;
typedef int32_t  sDWORD;
typedef uint32_t DWORD;
typedef int64_t  sQWORD;
typedef uint64_t QWORD;

// External CPU state (registers)
extern sDWORD dasmReg[32];
extern sDWORD dasmRegHi;
extern sDWORD dasmRegLo;

// Memory read/write functions (to be implemented in interpMemory.cpp)
extern sDWORD  iMemReadDWord(uint32_t addr);
extern uint32_t iMemReadWord(uint32_t addr);
extern uint8_t  iMemReadByte(uint32_t addr);
extern sQWORD  iMemReadQWord(uint32_t addr);

extern void iMemWriteDWord(sDWORD value, uint32_t addr);
extern void iMemWriteWord(uint16_t value, uint32_t addr);
extern void iMemWriteByte(uint8_t value, uint32_t addr);
extern void iMemWriteQWord(sQWORD value, uint32_t addr);

//-------------------- Memory operations --------------------
extern void interpOpLui(BYTE op0, DWORD imm);

// Add other memory operations as needed:
// - Load/Store Byte: interpOpLb, interpOpLbu, interpOpSb
// - Load/Store Half: interpOpLh, interpOpLhu, interpOpSh
// - Load/Store Word: interpOpLw, interpOpSw
// - Load/Store Double: interpOpLd, interpOpSd
// - Load/Store Conditional: interpOpLl, interpOpSc
// - Floating-point loads/stores: interpOpLwc1, interpOpSwc1, etc.
