#ifndef DYNA_FP
#define DYNA_FP
#include <switch.h>
#include <cstdint>
#include <cmath>
#include <cstring>

// Type definitions for portability
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

// ------------------------
// FP register operations
// ------------------------
extern WORD FP_COMP_S(BYTE *cp,BYTE Reg);
extern WORD FP_COMP_D(BYTE *cp,BYTE Reg);
extern WORD FP_STORE_STATUS(BYTE *cp);
extern WORD FP_SET_ROUNDCONTROL(BYTE *cp,DWORD Mode);
extern WORD FP_ROUND(BYTE *cp);

// ------------------------
// Load/store operations
// ------------------------
extern WORD FP_LOAD_S_REG(BYTE *cp,BYTE Reg);
extern WORD FP_LOAD_W_REG(BYTE *cp,BYTE Reg);
extern WORD FP_LOAD_L_REG(BYTE *cp,BYTE Reg);
extern WORD FP_LOAD_D_REG(BYTE *cp,BYTE Reg);
extern WORD FP_STORE_S_REG(BYTE *cp,BYTE Reg);
extern WORD FP_STORE_D_REG(BYTE *cp,BYTE Reg);
extern WORD FP_STORE_L_REG(BYTE *cp,BYTE Reg);
extern WORD FP_STORE_W_REG(BYTE *cp,BYTE Reg);

// ------------------------
// Arithmetic operations
// ------------------------
extern WORD FP_ADD(BYTE *cp);
extern WORD FP_SUB(BYTE *cp);
extern WORD FP_MUL(BYTE *cp);
extern WORD FP_DIV(BYTE *cp);
extern WORD FP_SQRT(BYTE *cp);
extern WORD FP_ABS(BYTE *cp);
extern WORD FP_NEG(BYTE *cp);
extern WORD FP_COS(BYTE *cp);
extern WORD FP_SIN(BYTE *cp);

// ------------------------
// Binary FP operations
// ------------------------
extern WORD dynaOpFPAddS(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);
extern WORD dynaOpFPAddD(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);
extern WORD dynaOpFPSubS(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);
extern WORD dynaOpFPSubD(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);
extern WORD dynaOpFPMulS(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);
extern WORD dynaOpFPMulD(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);
extern WORD dynaOpFPDivS(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);
extern WORD dynaOpFPDivD(BYTE *cp,BYTE op0,BYTE op1,BYTE op2);

// ------------------------
// Unary FP operations
// ------------------------
extern WORD dynaOpFPSqrtS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPSqrtD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPAbsS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPAbsD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPNegS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPNegD(BYTE *cp,BYTE op0,BYTE op1);

// ------------------------
// Conversions
// ------------------------
extern WORD dynaOpFPCvtSFromS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtSFromD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtSFromW(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtSFromL(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtDFromS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtDFromW(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtDFromL(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtWFromS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtWFromD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtWFromL(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtLFromS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtLFromD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCvtLFromW(BYTE *cp,BYTE op0,BYTE op1);

// ------------------------
// Rounding/trunc/floor/ceil
// ------------------------
extern WORD dynaOpFPRoundLS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPRoundLD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPRoundWS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPRoundWD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPTruncWS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPTruncWD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPTruncLS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPTruncLD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCeilLS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCeilLD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCeilWS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPCeilWD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPFloorWD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPFloorWS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPFloorLD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPFloorLS(BYTE *cp,BYTE op0,BYTE op1);

// ------------------------
// Memory moves
// ------------------------
extern WORD dynaOpFPMovS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPMovD(BYTE *cp,BYTE op0,BYTE op1);

// ------------------------
// FP comparisons
// ------------------------
extern WORD dynaOpFPEqualS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPLessThanS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPLessThanEqualS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPEqualD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPLessThanD(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPLessThanEqualD(BYTE *cp,BYTE op0,BYTE op1);

// ------------------------
// Other FP helpers
// ------------------------
extern WORD dynaOpFPSetFalse(BYTE *cp);
extern WORD dynaOpFPUnorderedS(BYTE *cp,BYTE op0,BYTE op1);
extern WORD dynaOpFPUnorderedD(BYTE *cp,BYTE op0,BYTE op1);

#endif
