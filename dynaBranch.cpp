#include "stdafx.h"
#include "DynaCompiler.h"
#include "dynaNative.h"
#include "dynaBranch_arm64.h"
#include "imain.h"

extern DWORD ForceInt;
extern void iCpuVSYNC();

WORD globalAudioIncRateWhole=10000;
WORD globalAudioIncRatePartial=10000;
WORD globalAudioIncRateFrac=10000;
WORD dynaNextAIWhole=0;
WORD dynaNextAIPartial=0;

void StopMe() { DWORD Stop=1; }

// ------------------------
// I-loop (ultra ARM64)
// ------------------------
WORD dynaOpILoop(BYTE *cp, DWORD NewPC) {
    WORD l = 0;
    l += INC_PC_COUNTER(cp+l);
    l += MainInstruction[dynaNextOpCode>>26](cp+l);
    l += STORE_DWORD_TO_RBANK(cp+l, NewPC, 242*8);
    l += CALL_FUNCTION(cp+l, (DWORD)iCpuVSYNC);
    l += RETURN(cp+l);
    return l;
}

// ------------------------
// Return from exception
// ------------------------
WORD dynaOpEret(BYTE *cp) {
    WORD l=0;
    l += INC_PC_COUNTER(cp+l);
    l += AND_REG_IMM(cp+l, CPR0_+12*8, 0xFFFFFFFD);
    l += STORE_DWORD_TO_RBANK(cp+l,242*8, CPR0_+14*8);
    l += STORE_REG(cp+l, NATIVE_0, 1972);
    l += STORE_REG(cp+l, NATIVE_0, 1944);
    l += RETURN(cp+l);
    return l;
}

// ------------------------
// Generic jump
// ------------------------
WORD dynaOpJ(BYTE *cp, DWORD NewPC) {
    WORD l = 0;
    DWORD Page = (NewPC & PAGE_MASK) >> PAGE_SHIFT;
    DWORD Offset = (NewPC & OFFSET_MASK) >> OFFSET_SHIFT;
    DWORD CompiledPC = dynaPageTable[Page].Offset[Offset];

    l += INC_PC_COUNTER(cp+l);
    l += MainInstruction[dynaNextOpCode>>26](cp+l);

    if(Page == dynaCurPage && ((NewPC & MEM_MASK) > (dynaPC & MEM_MASK))) {
        l += ARM64_JMP_LONG(cp+l, CompiledPC);
    } else {
        l += STORE_DWORD_TO_RBANK(cp+l, NewPC, 242*8);
        if(dynaIsInfinite(NewPC, dynaPC)) {
            theApp_LogMessage("Nuked iloop at %X", dynaPC);
            l += CALL_FUNCTION(cp+l, (DWORD)iCpuVSYNC);
            l += CALL_CHECKINTS(cp+l);
        }
        l += RETURN(cp+l);
    }
    return l;
}

// ------------------------
// Generic conditional branch template
// ------------------------
#define ARM64_BRANCH(opname, cmpinstr, branchinstr) \
WORD opname(BYTE *cp, BYTE op0, BYTE op1, DWORD NewPC) { \
    WORD l = 0; BYTE *fix; WORD fixlen; \
    l += INC_PC_COUNTER(cp+l); \
    l += LOAD_REG_FROM_RBANK(cp+l, NATIVE_0, op0*8); \
    l += XOR_REG_WITH_REG(cp+l, NATIVE_0, op1*8); \
    fix = cp+l; fixlen = l; \
    l += branchinstr(cp+l,0); \
    l += MainInstruction[dynaNextOpCode>>26](cp+l); \
    l += STORE_DWORD_TO_RBANK(cp+l, NewPC, 242*8); \
    l += RETURN(cp+l); \
    branchinstr(fix,(l-fixlen)-6); \
    return l; \
}

// ------------------------
// Standard branches
// ------------------------
ARM64_BRANCH(dynaOpBeq, CMP_REG_IMM, ARM64_JNE_LONG)
ARM64_BRANCH(dynaOpBne, CMP_REG_IMM, ARM64_JE_LONG)
ARM64_BRANCH(dynaOpBeql, CMP_REG_IMM, ARM64_JNE_SHORT)
ARM64_BRANCH(dynaOpBnel, CMP_REG_IMM, ARM64_JE_SHORT)

// ------------------------
// Zero/negative branches
// ------------------------
#define ARM64_BRANCH_ZERO(opname, cmpinstr, branchinstr) \
WORD opname(BYTE *cp, BYTE op0, DWORD NewPC) { \
    WORD l = 0; BYTE *fix1,*fix2; WORD len1,len2; \
    l += INC_PC_COUNTER(cp+l); \
    l += cmpinstr(cp+l,dynaMapToNative[op0].Reg,0); \
    fix1 = cp+l; len1 = l; \
    l += branchinstr(cp+l,0); \
    l += PUSH_DWORD(cp+l,NewPC); \
    fix2 = cp+l; len2 = l; \
    l += JMP_SHORT(cp+l,0); \
    branchinstr(fix1,(l-len1)-2); \
    l += PUSH_DWORD(cp+l,dynaPC+8); \
    JMP_SHORT(fix2,(l-len2)-2); \
    l += ARM64_POP_REG(cp+l,PC_PTR); \
    l += STORE_REG(cp+l,242,PC_PTR); \
    l += RETURN(cp+l); \
    return l; \
}

ARM64_BRANCH_ZERO(dynaOpBlez, ARM64_CMP_REG_IMM, ARM64_JG_SHORT)
ARM64_BRANCH_ZERO(dynaOpBgtz, ARM64_CMP_REG_IMM, ARM64_JLE_SHORT)
ARM64_BRANCH_ZERO(dynaOpBltz, ARM64_CMP_REG_IMM, ARM64_JGE_SHORT)
ARM64_BRANCH_ZERO(dynaOpBgez, ARM64_CMP_REG_IMM, ARM64_JL_SHORT)

// ------------------------
// Link branches (Bgezal/Bltzal/etc.)
// ------------------------
#define ARM64_BRANCH_LINK(opname, cmpinstr, branchinstr) \
WORD opname(BYTE *cp, BYTE op0, DWORD NewPC) { \
    WORD l = 0; BYTE *fix1,*fix2; WORD len1,len2; \
    l += INC_PC_COUNTER(cp+l); \
    l += cmpinstr(cp+l,dynaMapToNative[op0].Reg,0); \
    fix1 = cp+l; len1 = l; \
    l += branchinstr(cp+l,0); \
    l += PUSH_DWORD(cp+l,NewPC); \
    fix2 = cp+l; len2 = l; \
    l += JMP_SHORT(cp+l,0); \
    branchinstr(fix1,(l-len1)-2); \
    l += PUSH_DWORD(cp+l,dynaPC+8); \
    JMP_SHORT(fix2,(l-len2)-2); \
    l += MainInstruction[dynaNextOpCode>>26](cp+l); \
    l += STORE_DWORD_TO_RBANK(cp+l,dynaPC+8,31*8); \
    l += ARM64_POP_REG(cp+l,PC_PTR); \
    l += STORE_REG(cp+l,242,PC_PTR); \
    l += RETURN(cp+l); \
    return l; \
}

ARM64_BRANCH_LINK(dynaOpBgezal, ARM64_CMP_REG_IMM, ARM64_JL_SHORT)
ARM64_BRANCH_LINK(dynaOpBltzal, ARM64_CMP_REG_IMM, ARM64_JGE_SHORT)
ARM64_BRANCH_LINK(dynaOpBgezall, ARM64_CMP_REG_IMM, ARM64_JL_SHORT)
ARM64_BRANCH_LINK(dynaOpBltzall, ARM64_CMP_REG_IMM, ARM64_JGE_SHORT)

// ------------------------
// CCR branches (Bct, Bctl, Bcf, Bcfl)
// ------------------------
#define ARM64_BRANCH_CCR(opname, branchinstr) \
WORD opname(BYTE *cp,DWORD NewPC) { \
    WORD l = 0; BYTE *fix1,*fix2; WORD len1,len2; \
    l += INC_PC_COUNTER(cp+l); \
    l += LOAD_REG_FROM_RBANK(cp+l,NATIVE_0,CCR1_+31*8); \
    l += AND_REG_IMM(cp+l,NATIVE_0,0x00800000); \
    fix1 = cp+l; len1 = l; \
    l += branchinstr(cp+l,0); \
    l += PUSH_DWORD(cp+l,NewPC); \
    fix2 = cp+l; len2 = l; \
    l += JMP_SHORT(cp+l,0); \
    branchinstr(fix1,(l-len1)-2); \
    l += PUSH_DWORD(cp+l,dynaPC+8); \
    JMP_SHORT(fix2,(l-len2)-2); \
    l += MainInstruction[dynaNextOpCode>>26](cp+l); \
    l += ARM64_POP_REG(cp+l,PC_PTR); \
    l += STORE_REG(cp+l,242,PC_PTR); \
    l += RETURN(cp+l); \
    return l; \
}

ARM64_BRANCH_CCR(dynaOpBct, ARM64_JZ_SHORT)
ARM64_BRANCH_CCR(dynaOpBctl, ARM64_JZ_SHORT)
ARM64_BRANCH_CCR(dynaOpBcf, ARM64_JNZ_SHORT)
ARM64_BRANCH_CCR(dynaOpBcfl, ARM64_JNZ_SHORT)
