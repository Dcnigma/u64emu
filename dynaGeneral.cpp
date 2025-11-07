#include <cmath>
#include <cstdint>

#include "ki.h"
#include "DynaCompiler.h"
#include "dynaNative.h"
#include "dynaGeneral.h"

#define WIDTH_MULT 2
#define LOGIC64

using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;
using sDWORD = int32_t;
using QWORD = uint64_t;
using sQWORD = int64_t;

extern RegisterBank *r;

// ------------------------- Shift Operations -------------------------

// SRA rd, rt, shamt
WORD dynaOpSra(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += SRA_REG_IMM(cp+l, NATIVE_0, op2);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

// SRL rd, rt, shamt
WORD dynaOpSrl(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += SRL_REG_IMM(cp+l, NATIVE_0, op2);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

// SLL rd, rt, shamt
WORD dynaOpSll(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += SLL_REG_IMM(cp+l, NATIVE_0, op2);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

// SRA rd, rd, shamt (register-equal)
WORD dynaOpSraEq(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += SAR_RBANK_IMM(cp+l, op0*8, op2);
    return l;
}

// SRL rd, rd, shamt (register-equal)
WORD dynaOpSrlEq(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += SHR_RBANK_IMM(cp+l, op0*8, op2);
    return l;
}

// SLL rd, rd, shamt (register-equal)
WORD dynaOpSllEq(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += SHL_RBANK_IMM(cp+l, op0*8, op2);
    return l;
}

// Variable shifts
WORD dynaOpSraV(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += dynaLoadRegForce(cp+l, op2, NATIVE_2);
    l += AND_REG_IMM(cp+l, NATIVE_2, 0x1f);
    l += SRA_REG_REG(cp+l, NATIVE_0, NATIVE_2);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

WORD dynaOpSrlV(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += dynaLoadRegForce(cp+l, op2, NATIVE_2);
    l += AND_REG_IMM(cp+l, NATIVE_2, 0x1f);
    l += SRL_REG_REG(cp+l, NATIVE_0, NATIVE_2);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

WORD dynaOpSllV(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += dynaLoadRegForce(cp+l, op2, NATIVE_2);
    l += AND_REG_IMM(cp+l, NATIVE_2, 0x1f);
    l += SLL_REG_REG(cp+l, NATIVE_0, NATIVE_2);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

// ------------------------- Move Operations -------------------------

WORD dynaOpMfHi(BYTE *cp, BYTE op0) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += MEM_TO_MEM_QWORD(cp+l, op0*8, HI);
    return l;
}

WORD dynaOpMfLo(BYTE *cp, BYTE op0) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += MEM_TO_MEM_QWORD(cp+l, op0*8, LO);
    return l;
}

WORD dynaOpMtHi(BYTE *cp, BYTE op0) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += MEM_TO_MEM_QWORD(cp+l, HI, op0*8);
    return l;
}

WORD dynaOpMtLo(BYTE *cp, BYTE op0) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += MEM_TO_MEM_QWORD(cp+l, LO, op0*8);
    return l;
}

WORD dynaOpRegMove(BYTE *cp, BYTE op0, BYTE op1) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += MEM_TO_MEM_QWORD(cp+l, op0*8, op1*8);
    return l;
}

// ------------------------- Arithmetic -------------------------

WORD dynaOpAdd(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += ADD_DWORD_TO_REG(cp+l, NATIVE_0, op2*8);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

WORD dynaOpAddEqual(BYTE *cp, BYTE op0, BYTE op1) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += dynaLoadReg(cp+l, op1);
    l += CONVERT_TO_QWORD(cp+l);
    l += ADD_REG_TO_DWORD(cp+l, dynaMapToNative[op1].Reg, op0*8);
    l += ADC_REG_TO_DWORD(cp+l, NATIVE_3, op0*8+4);
    dynaKillNative(dynaMapToNative[op1].Reg);
    return l;
}

WORD dynaOpSub(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += SUB_DWORD_FROM_REG(cp+l, NATIVE_0, op2*8);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

WORD dynaOpSubEqual(BYTE *cp, BYTE op0, BYTE op1) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += dynaLoadReg(cp+l, op1);
    l += CONVERT_TO_QWORD(cp+l);
    l += SUB_REG_FROM_DWORD(cp+l, dynaMapToNative[op1].Reg, op0*8);
    l += SBB_REG_FROM_DWORD(cp+l, NATIVE_3, op0*8+4);
    dynaKillNative(dynaMapToNative[op1].Reg);
    return l;
}

// ------------------------- Bitwise -------------------------

WORD dynaOpAnd(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += AND_REG_REG(cp+l, NATIVE_0, op2*8);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

WORD dynaOpOr(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += OR_REG_REG(cp+l, NATIVE_0, op2*8);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

WORD dynaOpXor(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += XOR_REG_REG(cp+l, NATIVE_0, op2*8);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

WORD dynaOpXori(BYTE *cp, BYTE op0, BYTE op1, DWORD imm) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += LOAD_GPR(cp+l, op1*8, op0*8);
    l += XOR_REG_IMM(cp+l, NATIVE_0, imm);
    l += STORE_SE_D_GPR(cp+l);
    return l;
}

// ------------------------- Comparison -------------------------

WORD dynaOpSlt(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    sDWORD val1 = *(sDWORD*)&r->GPR[op1*WIDTH_MULT];
    sDWORD val2 = *(sDWORD*)&r->GPR[op2*WIDTH_MULT];
    r->GPR[op0*WIDTH_MULT] = (val1 < val2) ? 1 : 0;
    return l;
}

WORD dynaOpSltU(BYTE *cp, BYTE op0, BYTE op1, BYTE op2) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    DWORD val1 = *(DWORD*)&r->GPR[op1*WIDTH_MULT];
    DWORD val2 = *(DWORD*)&r->GPR[op2*WIDTH_MULT];
    r->GPR[op0*WIDTH_MULT] = (val1 < val2) ? 1 : 0;
    return l;
}

WORD dynaOpSltI(BYTE *cp, BYTE op0, BYTE op1, sDWORD imm) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    sDWORD val1 = *(sDWORD*)&r->GPR[op1*WIDTH_MULT];
    r->GPR[op0*WIDTH_MULT] = (val1 < imm) ? 1 : 0;
    return l;
}

// ------------------------- Branch -------------------------

WORD dynaOpBeq(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    if (*(DWORD*)&r->GPR[op0*WIDTH_MULT] == *(DWORD*)&r->GPR[op1*WIDTH_MULT])
        r->PC += offset;
    return l;
}

WORD dynaOpBne(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    if (*(DWORD*)&r->GPR[op0*WIDTH_MULT] != *(DWORD*)&r->GPR[op1*WIDTH_MULT])
        r->PC += offset;
    return l;
}

WORD dynaOpBltz(BYTE *cp, BYTE op0, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    if (*(sDWORD*)&r->GPR[op0*WIDTH_MULT] < 0)
        r->PC += offset;
    return l;
}

WORD dynaOpBgez(BYTE *cp, BYTE op0, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    if (*(sDWORD*)&r->GPR[op0*WIDTH_MULT] >= 0)
        r->PC += offset;
    return l;
}

// ------------------------- Memory -------------------------

WORD dynaOpLui(BYTE *cp, BYTE op0, DWORD imm) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    r->GPR[op0*WIDTH_MULT] = imm << 16;
    return l;
}

WORD dynaOpLw(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    DWORD addr = *(DWORD*)&r->GPR[op1*WIDTH_MULT] + offset;
    r->GPR[op0*WIDTH_MULT] = *((DWORD*)addr);
    return l;
}

WORD dynaOpSw(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    DWORD addr = *(DWORD*)&r->GPR[op1*WIDTH_MULT] + offset;
    *((DWORD*)addr) = *(DWORD*)&r->GPR[op0*WIDTH_MULT];
    return l;
}

WORD dynaOpLb(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    DWORD addr = *(DWORD*)&r->GPR[op1*WIDTH_MULT] + offset;
    r->GPR[op0*WIDTH_MULT] = (sBYTE)(*((BYTE*)addr));
    return l;
}

WORD dynaOpSb(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    DWORD addr = *(DWORD*)&r->GPR[op1*WIDTH_MULT] + offset;
    *((BYTE*)addr) = *(BYTE*)&r->GPR[op0*WIDTH_MULT];
    return l;
}

WORD dynaOpLh(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    DWORD addr = *(DWORD*)&r->GPR[op1*WIDTH_MULT] + offset;
    r->GPR[op0*WIDTH_MULT] = (sWORD)(*((WORD*)addr));
    return l;
}

WORD dynaOpSh(BYTE *cp, BYTE op0, BYTE op1, sDWORD offset) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    DWORD addr = *(DWORD*)&r->GPR[op1*WIDTH_MULT] + offset;
    *((WORD*)addr) = *(WORD*)&r->GPR[op0*WIDTH_MULT];
    return l;
}

// ------------------------- Multiplication / Division -------------------------

void dynaHelpDMult(DWORD op0, DWORD op1) {
    sQWORD x = (*(sQWORD*)&r->GPR[op0*WIDTH_MULT]) * (*(sQWORD*)&r->GPR[op1*WIDTH_MULT]);
    r->Lo = (sDWORD)x;
    r->Hi = (sDWORD)(x >> 32);
}

void dynaHelpDMultU(DWORD op0, DWORD op1) {
    QWORD x = (*(QWORD*)&r->GPR[op0*WIDTH_MULT]) * (*(QWORD*)&r->GPR[op1*WIDTH_MULT]);
    r->Lo = (DWORD)x;
    r->Hi = (DWORD)(x >> 32);
}

void dynaHelpDDiv(DWORD op0, DWORD op1) {
    sQWORD denom = *(sQWORD*)&r->GPR[op1*WIDTH_MULT];
    if (denom != 0) {
        sQWORD num = *(sQWORD*)&r->GPR[op0*WIDTH_MULT];
        r->Lo = (sDWORD)(num / denom);
        r->Hi = (sDWORD)(num % denom);
    }
}

void dynaHelpDDivU(DWORD op0, DWORD op1) {
    QWORD denom = *(QWORD*)&r->GPR[op1*WIDTH_MULT];
    if (denom != 0) {
        QWORD num = *(QWORD*)&r->GPR[op0*WIDTH_MULT];
        r->Lo = (DWORD)(num / denom);
        r->Hi = (DWORD)(num % denom);
    }
}

// ------------------------- Miscellaneous -------------------------

WORD dynaOpNop(BYTE *cp) {
    return INC_PC_COUNTER(cp);
}
