#include "stdafx.h"
#include "math.h"
#include "ki.h"

#include "DynaCompiler.h"
#include "dynaNative.h"
#include "dynaBranch_arm64.h"
#include "imain.h"

extern DWORD ForceInt;

WORD globalAudioIncRateWhole = 10000;
WORD globalAudioIncRatePartial = 10000;
WORD globalAudioIncRateFrac = 10000;
WORD dynaNextAIWhole = 0;
WORD dynaNextAIPartial = 0;

extern void iCpuVSYNC();

void StopMe() {
    DWORD Stop = 1;
}

bool dynaIsInfinite(DWORD target, DWORD pc) {
    (void)target; (void)pc;
    return false;
}

// --------------------------------------------------
// Loop instruction
// --------------------------------------------------
WORD dynaOpILoop(BYTE* cp, DWORD NewPC) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp + l);
    iOpCode = dynaNextOpCode;
    l += MainInstruction[dynaNextOpCode >> 26](cp + l);
    l += STORE_DWORD_TO_RBANK(cp + l, NewPC, 242 * 8);
    l += CALL_FUNCTION(cp + l, (DWORD)iCpuVSYNC);
    l += RETURN(cp + l);
    return l;
}

// --------------------------------------------------
// Return from exception
// --------------------------------------------------
WORD dynaOpEret(BYTE* cp) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp + l);
    l += AND_REG_IMM(cp + l, 0xfffffffd, CPR0_ + 12 * 8);
    l += MEM_TO_MEM_DWORD(cp + l, 242 * 8, CPR0_ + 14 * 8);
    ZERO_REG(cp + l, NATIVE_0);
    l += STORE_REG_TO_RBANK(cp + l, NATIVE_0, 1972);
    l += STORE_REG_TO_RBANK(cp + l, NATIVE_0, 1944);
    l += RETURN(cp + l);
    return l;
}

// --------------------------------------------------
// Jump instructions
// --------------------------------------------------
WORD dynaOpJ(BYTE* cp, DWORD NewPC) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp + l);
    iOpCode = dynaNextOpCode;
    DWORD Page = (NewPC & PAGE_MASK) >> PAGE_SHIFT;
    DWORD Offset = (NewPC & OFFSET_MASK) >> OFFSET_SHIFT;
    DWORD CompiledPC = (DWORD)dynaPageTable[Page].Offset[Offset];

    if (dynaIsInfinite(NewPC, dynaPC)) {
        l += CALL_FUNCTION(cp + l, (DWORD)iCpuVSYNC);
        l += CALL_CHECKINTS(cp + l);
    }

    l += ARM64_JMP_LONG(cp + l, CompiledPC);
    return l;
}

WORD dynaOpJr(BYTE* cp, BYTE op0) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp + l);
    l += STORE_DWORD_TO_RBANK(cp + l, dynaPC + 4, 242 * 8);
    l += LOAD_REG_FROM_RBANK(cp + l, NATIVE_0, op0 * 8);
    l += PUSH_REGISTER(cp + l, NATIVE_0);
    l += MainInstruction[dynaNextOpCode >> 26](cp + l);
    l += POP_REGISTER(cp + l, PC_PTR);
    l += STORE_REG(cp + l, 242, PC_PTR);
    l += RETURN(cp + l);
    return l;
}

WORD dynaOpJalr(BYTE* cp, BYTE op0, BYTE op1) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp + l);
    l += LOAD_REG_FROM_RBANK(cp + l, NATIVE_0, op1 * 8);
    l += PUSH_REGISTER(cp + l, NATIVE_0);
    l += MainInstruction[dynaNextOpCode >> 26](cp + l);
    l += STORE_DWORD_TO_RBANK(cp + l, dynaPC + 8, op0 * 8);
    l += POP_REGISTER(cp + l, PC_PTR);
    l += STORE_REG(cp + l, 242, PC_PTR);
    l += RETURN(cp + l);
    return l;
}

WORD dynaOpJal(BYTE* cp, DWORD NewPC) {
    WORD l = 0;
    l += STORE_DWORD_TO_RBANK(cp + l, dynaPC, 242 * 8);
    l += CALL_CHECKINTS(cp + l);

    l += LOAD_REG_FROM_RBANK(cp + l, PC_PTR, 242 * 8);
    l += CMP_REG_IMM(cp + l, PC_PTR, 0x80000180);
    BYTE* fixup1Ptr = cp + l;
    WORD fixup1Len = l;
    l += JNE_SHORT(cp + l, 0);
    l += RETURN(cp + l);
    JNE_SHORT(fixup1Ptr, (l - fixup1Len) - 2);

    iOpCode = dynaNextOpCode;
    l += MainInstruction[dynaNextOpCode >> 26](cp + l);

    DWORD Page = (NewPC & PAGE_MASK) >> PAGE_SHIFT;
    DWORD Offset = (NewPC & OFFSET_MASK) >> OFFSET_SHIFT;
    DWORD CompiledPC = (DWORD)dynaPageTable[Page].Offset[Offset];

    STORE_DWORD_TO_RBANK(cp + l, NewPC, 242 * 8);
    STORE_DWORD_TO_RBANK(cp + l, dynaPC + 8, 31 * 8);
    l += ARM64_JMP_LONG(cp + l, CompiledPC);

    return l;
}

// --------------------------------------------------
// Standard branches
// --------------------------------------------------
#define DEFINE_BRANCH_FN(name, macro) \
WORD name(BYTE* cp, BYTE op0, BYTE op1, DWORD NewPC) { \
    WORD l = 0; \
    l += dynaLoadRegPair(cp + l, op0); \
    XOR_REG_WITH_REG(cp + l, NATIVE_0, op1 * 8); \
    BYTE* fixupPtr = cp + l; \
    l += macro(cp + l, 0); \
    iOpCode = dynaNextOpCode; \
    l += MainInstruction[dynaNextOpCode >> 26](cp + l); \
    STORE_DWORD_TO_RBANK(cp + l, NewPC, 242 * 8); \
    l += RETURN(cp + l); \
    macro(fixupPtr, (l - 6)); \
    return l; \
}

DEFINE_BRANCH_FN(dynaOpBeq, ARM64_JNE_LONG)
DEFINE_BRANCH_FN(dynaOpBne, ARM64_JE_LONG)
DEFINE_BRANCH_FN(dynaOpBeql, ARM64_JNE_SHORT)
DEFINE_BRANCH_FN(dynaOpBnel, ARM64_JE_SHORT)

// --------------------------------------------------
// Single-register conditional branches
// --------------------------------------------------
#define DEFINE_BRANCH1_FN(name, cmpMacro) \
WORD name(BYTE* cp, BYTE op0, DWORD NewPC) { \
    WORD l = 0; \
    l += dynaLoadReg(cp + l, op0); \
    cmpMacro(cp + l, dynaMapToNative[op0].Reg, 0); \
    BYTE* fixupPtr = cp + l; \
    l += ARM64_JGE_SHORT(cp + l, 0); \
    l += PUSH_DWORD(cp + l, NewPC); \
    iOpCode = dynaNextOpCode; \
    l += MainInstruction[dynaNextOpCode >> 26](cp + l); \
    BYTE* fixup2Ptr = cp + l; \
    l += JMP_SHORT(cp + l, 0); \
    JMP_SHORT(fixupPtr, (l - 2)); \
    l += PUSH_DWORD(cp + l, dynaPC + 8); \
    JMP_SHORT(fixup2Ptr, (l - 2)); \
    l += POP_REGISTER(cp + l, PC_PTR); \
    l += STORE_REG(cp + l, 242, PC_PTR); \
    l += RETURN(cp + l); \
    return l; \
}

DEFINE_BRANCH1_FN(dynaOpBlez, CMP_REG_IMM)
DEFINE_BRANCH1_FN(dynaOpBgtz, CMP_REG_IMM)
DEFINE_BRANCH1_FN(dynaOpBltz, CMP_REG_IMM)
DEFINE_BRANCH1_FN(dynaOpBgez, CMP_REG_IMM)
DEFINE_BRANCH1_FN(dynaOpBlezl, CMP_REG_IMM)
DEFINE_BRANCH1_FN(dynaOpBgtzl, CMP_REG_IMM)
DEFINE_BRANCH1_FN(dynaOpBltzl, CMP_REG_IMM)
DEFINE_BRANCH1_FN(dynaOpBgezl, CMP_REG_IMM)

// --------------------------------------------------
// Branch-and-link templates
// --------------------------------------------------
#define DEFINE_BRANCH_LINK_FN(name) \
WORD name(BYTE* cp, BYTE op0, DWORD NewPC) { \
    WORD l = 0; \
    l += dynaLoadRegPair(cp + l, op0); \
    CMP_REG_IMM(cp + l, NATIVE_3, 0); \
    BYTE* fixupPtr = cp + l; \
    l += ARM64_JGE_SHORT(cp + l, 0); \
    l += PUSH_DWORD(cp + l, NewPC); \
    BYTE* fixup2Ptr = cp + l; \
    l += JMP_SHORT(cp + l, 0); \
    JMP_SHORT(fixupPtr, (l - 2)); \
    l += PUSH_DWORD(cp + l, dynaPC + 8); \
    JMP_SHORT(fixup2Ptr, (l - 2)); \
    iOpCode = dynaNextOpCode; \
    l += MainInstruction[dynaNextOpCode >> 26](cp + l); \
    STORE_DWORD_TO_RBANK(cp + l, dynaPC + 8, 31 * 8); \
    l += POP_REGISTER(cp + l, PC_PTR); \
    l += STORE_REG(cp + l, 242, PC_PTR); \
    l += RETURN(cp + l); \
    return l; \
}

DEFINE_BRANCH_LINK_FN(dynaOpBltzal)
DEFINE_BRANCH_LINK_FN(dynaOpBgezal)
DEFINE_BRANCH_LINK_FN(dynaOpBltzall)
DEFINE_BRANCH_LINK_FN(dynaOpBgezall)

// --------------------------------------------------
// CCR branches
// --------------------------------------------------
#define DEFINE_CCR_BRANCH_FN(name, macro) \
WORD name(BYTE* cp, DWORD NewPC) { \
    WORD l = 0; \
    l += LOAD_REG_FROM_RBANK(cp + l, NATIVE_0, CCR1_ + 31 * 8); \
    AND_REG_IMM(cp + l, NATIVE_0, 0x00800000); \
    BYTE* fixupPtr = cp + l; \
    l += macro(cp + l, 0); \
    l += PUSH_DWORD(cp + l, NewPC); \
    l += PUSH_DWORD(cp + l, dynaPC + 8); \
    iOpCode = dynaNextOpCode; \
    l += MainInstruction[dynaNextOpCode >> 26](cp + l); \
    l += POP_REGISTER(cp + l, PC_PTR); \
    l += STORE_REG(cp + l, 242, PC_PTR); \
    l += RETURN(cp + l); \
    macro(fixupPtr, (l - 2)); \
    return l; \
}

DEFINE_CCR_BRANCH_FN(dynaOpBct, ARM64_JZ_SHORT)
DEFINE_CCR_BRANCH_FN(dynaOpBctl, ARM64_JZ_SHORT)
DEFINE_CCR_BRANCH_FN(dynaOpBcf, ARM64_JNZ_SHORT)
DEFINE_CCR_BRANCH_FN(dynaOpBcfl, ARM64_JNZ_SHORT)
