#pragma once
#include <switch.h>
#include <stdint.h>

// Use standard types
typedef uint8_t  BYTE;
typedef int32_t  sDWORD;
typedef uint32_t DWORD;
typedef int64_t  sQWORD;
typedef uint64_t QWORD;

// External CPU / register state
extern sDWORD dasmReg[32];
extern sDWORD dasmRegHi;
extern sDWORD dasmRegLo;
extern QWORD  dasmCPR0[32];
extern QWORD  dasmCPR1[32];
extern QWORD  dasmCPC0[32];
extern QWORD  dasmCPC1[32];

//-------------------- Shift operations --------------------
extern void interpOpSra(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSrl(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSll(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSrav(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSrlv(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSllv(BYTE op0, BYTE op1, BYTE op2);

//-------------------- Move operations --------------------
extern void interpOpMfHi(BYTE op0);
extern void interpOpMfLo(BYTE op0);
extern void interpOpMtHi(BYTE op0);
extern void interpOpMtLo(BYTE op0);
extern void interpOpRegMove(BYTE op0, BYTE op1);
extern void interpOpRegMoveD(BYTE op0, BYTE op1);

//-------------------- Arithmetic --------------------
extern void interpOpAdd(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSub(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpAnd(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpOr(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpXor(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpNor(BYTE op0, BYTE op1, BYTE op2);

//-------------------- Arithmetic assignment --------------------
extern void interpOpAddEqual(BYTE op0, BYTE op1);
extern void interpOpSubEqual(BYTE op0, BYTE op1);
extern void interpOpAndEqual(BYTE op0, BYTE op1);
extern void interpOpOrEqual(BYTE op0, BYTE op1);
extern void interpOpXorEqual(BYTE op0, BYTE op1);
extern void interpOpNorEqual(BYTE op0, BYTE op1);

//-------------------- Multiply/Divide --------------------
extern void interpOpMult(BYTE op0, BYTE op1);
extern void interpOpMultU(BYTE op0, BYTE op1);
extern void interpOpDiv(BYTE op0, BYTE op1);
extern void interpOpDivU(BYTE op0, BYTE op1);
extern void interpOpDMult(BYTE op0, BYTE op1);
extern void interpOpDMultU(BYTE op0, BYTE op1);
extern void interpOpDDiv(BYTE op0, BYTE op1);
extern void interpOpDDivU(BYTE op0, BYTE op1);

//-------------------- Compare --------------------
extern void interpOpSlt(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSltU(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpSlti(BYTE op0, BYTE op1, DWORD imm);
extern void interpOpSltiU(BYTE op0, BYTE op1, DWORD imm);

//-------------------- Immediate operations --------------------
extern void interpOpDAddI(BYTE op0, BYTE op1, DWORD imm);
extern void interpOpDAddIEqual(BYTE op0, DWORD imm);
extern void interpOpAddI(BYTE op0, BYTE op1, DWORD op2);
extern void interpOpAddIEqual(BYTE op0, DWORD op2);
extern void interpOpLoadI(BYTE op0, DWORD op2);

//-------------------- Doubleword shifts --------------------
extern void interpOpDSra(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSraV(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSra32(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSrl(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSrlV(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSrl32(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSll(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSllV(BYTE op0, BYTE op1, BYTE op2);
extern void interpOpDSll32(BYTE op0, BYTE op1, BYTE op2);

//-------------------- Control registers --------------------
extern void interpOpRs0Mf(BYTE op0, BYTE op1);
extern void interpOpRs0DMf(BYTE op0, BYTE op1);
extern void interpOpRs0Mt(BYTE op0, BYTE op1);
extern void interpOpRs0DMt(BYTE op0, BYTE op1);

extern void interpOpRs1Mf(BYTE op0, BYTE op1);
extern void interpOpRs1DMf(BYTE op0, BYTE op1);
extern void interpOpRs1Mt(BYTE op0, BYTE op1);
extern void interpOpRs1DMt(BYTE op0, BYTE op1);

extern void interpOpRs0Ct(BYTE op0, BYTE op1);
extern void interpOpRs1Ct(BYTE op0, BYTE op1);
extern void interpOpRs0DCt(BYTE op0, BYTE op1);
extern void interpOpRs1DCt(BYTE op0, BYTE op1);

//-------------------- Immediate bitwise --------------------
extern void interpOpAndI(BYTE op0, BYTE op1, DWORD imm);
extern void interpOpOrI(BYTE op0, BYTE op1, DWORD imm);
extern void interpOpXorI(BYTE op0, BYTE op1, DWORD imm);
extern void interpOpAndIEqual(BYTE op0, DWORD imm);
extern void interpOpOrIEqual(BYTE op0, DWORD imm);
extern void interpOpXorIEqual(BYTE op0, DWORD imm);
