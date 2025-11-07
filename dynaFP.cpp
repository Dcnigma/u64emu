#include <cstring>
#include <cmath>
#include "dynaFP.h"
#include "DynaCompiler.h"
#include "dynaNative.h"
#include "iMain.h"
#include <switch.h>

// ------------------------
// Floating point register offset
// ------------------------
#define FPR_OFFSET 1792-128

// ------------------------
// FP register operations
// ------------------------
WORD FP_COMP_S(BYTE *cp, BYTE Reg) {
    // Pure ARM64 dynamic compilation placeholder
    return 0;
}

WORD FP_COMP_D(BYTE *cp, BYTE Reg) {
    return 0;
}

WORD FP_STORE_STATUS(BYTE *cp) { return 0; }
WORD FP_SET_ROUNDCONTROL(BYTE *cp, DWORD Mode) { return 0; }
WORD FP_ROUND(BYTE *cp) { return 0; }

// ------------------------
// Load/store operations
// ------------------------
WORD FP_LOAD_S_REG(BYTE *cp,BYTE Reg) { return 0; }
WORD FP_LOAD_D_REG(BYTE *cp,BYTE Reg) { return 0; }
WORD FP_LOAD_W_REG(BYTE *cp,BYTE Reg) { return 0; }
WORD FP_LOAD_L_REG(BYTE *cp,BYTE Reg) { return 0; }
WORD FP_STORE_S_REG(BYTE *cp,BYTE Reg) { return 0; }
WORD FP_STORE_D_REG(BYTE *cp,BYTE Reg) { return 0; }
WORD FP_STORE_W_REG(BYTE *cp,BYTE Reg) { return 0; }
WORD FP_STORE_L_REG(BYTE *cp,BYTE Reg) { return 0; }

// ------------------------
// Arithmetic operations
// ------------------------
WORD FP_ADD(BYTE *cp){ return 0; }
WORD FP_SUB(BYTE *cp){ return 0; }
WORD FP_MUL(BYTE *cp){ return 0; }
WORD FP_DIV(BYTE *cp){ return 0; }
WORD FP_SQRT(BYTE *cp){ return 0; }
WORD FP_ABS(BYTE *cp){ return 0; }
WORD FP_NEG(BYTE *cp){ return 0; }
WORD FP_COS(BYTE *cp){ return 0; }
WORD FP_SIN(BYTE *cp){ return 0; }

#define FP_BIN_OP(Name,Load,Op,Store) \
WORD Name(BYTE *cp,BYTE op0,BYTE op1,BYTE op2){ \
    WORD l=0; l+=INC_PC_COUNTER(cp); \
    l+=Load(cp+l,op1); \
    l+=Op(cp+l,op2); \
    l+=Store(cp+l,op0); \
    return l; \
}

FP_BIN_OP(dynaOpFPAddS, FP_LOAD_S_REG, FP_ADD, FP_STORE_S_REG)
FP_BIN_OP(dynaOpFPAddD, FP_LOAD_D_REG, FP_ADD, FP_STORE_D_REG)
FP_BIN_OP(dynaOpFPSubS, FP_LOAD_S_REG, FP_SUB, FP_STORE_S_REG)
FP_BIN_OP(dynaOpFPSubD, FP_LOAD_D_REG, FP_SUB, FP_STORE_D_REG)
FP_BIN_OP(dynaOpFPMulS, FP_LOAD_S_REG, FP_MUL, FP_STORE_S_REG)
FP_BIN_OP(dynaOpFPMulD, FP_LOAD_D_REG, FP_MUL, FP_STORE_D_REG)
FP_BIN_OP(dynaOpFPDivS, FP_LOAD_S_REG, FP_DIV, FP_STORE_S_REG)
FP_BIN_OP(dynaOpFPDivD, FP_LOAD_D_REG, FP_DIV, FP_STORE_D_REG)

#define FP_UNARY_OP(Name,Load,Op,Store) \
WORD Name(BYTE *cp,BYTE op0,BYTE op1){ \
    WORD l=0; l+=INC_PC_COUNTER(cp); \
    l+=Load(cp+l,op1); \
    l+=Op(cp+l); \
    l+=Store(cp+l,op0); \
    return l; \
}

FP_UNARY_OP(dynaOpFPSqrtS, FP_LOAD_S_REG, FP_SQRT, FP_STORE_S_REG)
FP_UNARY_OP(dynaOpFPSqrtD, FP_LOAD_D_REG, FP_SQRT, FP_STORE_D_REG)
FP_UNARY_OP(dynaOpFPAbsS, FP_LOAD_S_REG, FP_ABS, FP_STORE_S_REG)
FP_UNARY_OP(dynaOpFPAbsD, FP_LOAD_D_REG, FP_ABS, FP_STORE_D_REG)
FP_UNARY_OP(dynaOpFPNegS, FP_LOAD_S_REG, FP_NEG, FP_STORE_S_REG)
FP_UNARY_OP(dynaOpFPNegD, FP_LOAD_D_REG, FP_NEG, FP_STORE_D_REG)

// ------------------------
// Conversions
// ------------------------
#define FP_CVT_OP(Name,Load,Store) \
WORD Name(BYTE *cp,BYTE op0,BYTE op1){ \
    WORD l=0; l+=INC_PC_COUNTER(cp); \
    l+=Load(cp+l,op1); \
    l+=Store(cp+l,op0); \
    return l; \
}

FP_CVT_OP(dynaOpFPCvtSFromS, FP_LOAD_S_REG, FP_STORE_S_REG)
FP_CVT_OP(dynaOpFPCvtSFromD, FP_LOAD_D_REG, FP_STORE_S_REG)
FP_CVT_OP(dynaOpFPCvtSFromW, FP_LOAD_W_REG, FP_STORE_S_REG)
FP_CVT_OP(dynaOpFPCvtSFromL, FP_LOAD_L_REG, FP_STORE_S_REG)
FP_CVT_OP(dynaOpFPCvtDFromS, FP_LOAD_S_REG, FP_STORE_D_REG)
FP_CVT_OP(dynaOpFPCvtDFromW, FP_LOAD_W_REG, FP_STORE_D_REG)
FP_CVT_OP(dynaOpFPCvtDFromL, FP_LOAD_L_REG, FP_STORE_D_REG)
FP_CVT_OP(dynaOpFPCvtWFromS, FP_LOAD_S_REG, FP_STORE_W_REG)
FP_CVT_OP(dynaOpFPCvtWFromD, FP_LOAD_D_REG, FP_STORE_W_REG)
FP_CVT_OP(dynaOpFPCvtWFromL, FP_LOAD_L_REG, FP_STORE_W_REG)
FP_CVT_OP(dynaOpFPCvtLFromS, FP_LOAD_S_REG, FP_STORE_L_REG)
FP_CVT_OP(dynaOpFPCvtLFromD, FP_LOAD_D_REG, FP_STORE_L_REG)
FP_CVT_OP(dynaOpFPCvtLFromW, FP_LOAD_W_REG, FP_STORE_L_REG)

// ------------------------
// Rounding/trunc/floor/ceil
// ------------------------
// (ARM64 fcvt instructions, plug-and-play)
#include "dynaFP_ARM64_Round.h" // <--- include the ARM64 rounding/trunc/floor/ceil implementations previously provided

// ------------------------
// FP move memory
// ------------------------
WORD dynaOpFPMovS(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPMovD(BYTE *cp,BYTE op0,BYTE op1){ return 0; }

// ------------------------
// FP comparisons
// ------------------------
WORD dynaOpFPEqualS(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPLessThanS(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPLessThanEqualS(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPEqualD(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPLessThanD(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPLessThanEqualD(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPSetFalse(BYTE *cp){ return 0; }
WORD dynaOpFPUnorderedS(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
WORD dynaOpFPUnorderedD(BYTE *cp,BYTE op0,BYTE op1){ return 0; }
