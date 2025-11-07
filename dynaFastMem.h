#ifndef DYNA_FAST_MEM
#define DYNA_FAST_MEM

#include <switch.h>
#include "dynaMemory.h"

// ---------------- Dynamic Compile Builders ----------------
extern int dynaCompileBuilderSB(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSH(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSW(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSD(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLD(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLW(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLWU(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLB(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLBU(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLH(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLHU(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLWC1(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLDC1(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLWC2(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLDC2(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLL(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLLD(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLDL(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLWL(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLDR(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderLWR(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);

extern int dynaCompileBuilderSWC1(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSDC1(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSWC2(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSDC2(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSC(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSCD(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSDL(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSWL(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSDR(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);
extern int dynaCompileBuilderSWR(BYTE *codeptr, BYTE op0, BYTE op1, uintptr_t Imm);

// ---------------- Dynamic Runtime Builders ----------------
extern void dynaRuntimeBuilderSW(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSB(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSH(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSD(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLW(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLWU(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLD(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLB(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLBU(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLH(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLHU(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLWC1(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLL(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLLD(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLDC1(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaHelperLDL(uintptr_t address, uintptr_t op0);
extern void dynaHelperLWL(uintptr_t address, uintptr_t op0);
extern void dynaHelperLDR(uintptr_t address, uintptr_t op0);
extern void dynaHelperLWR(uintptr_t address, uintptr_t op0);

extern void dynaHelperSDL(uintptr_t address, uintptr_t op0);
extern void dynaHelperSWL(uintptr_t address, uintptr_t op0);
extern void dynaHelperSDR(uintptr_t address, uintptr_t op0);
extern void dynaHelperSWR(uintptr_t address, uintptr_t op0);

extern void dynaRuntimeBuilderLDL(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLWL(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLDR(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLWR(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLDC2(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderLWC2(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);

extern void dynaRuntimeBuilderSDL(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSWL(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSDR(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSWR(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSDC2(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSWC2(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);

extern void dynaRuntimeBuilderSWC1(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSDC1(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSWC2(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSDC2(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSC(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);
extern void dynaRuntimeBuilderSCD(uintptr_t codeptr, uintptr_t op0, uintptr_t op1, uintptr_t Imm, uintptr_t StackPointer);

#endif
