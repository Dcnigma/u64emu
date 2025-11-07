#include <switch.h>
#include "math.h"
#include "ki.h"
#include "iCPU.h"

#include "DynaCompiler.h"
#include "dynaNative.h"
#include "dynaBranchSP.h"

extern void iCpuCatch();

#define DO_CHECKn (((NewPC&MEM_MASK)<=(dynaPC&MEM_MASK)))
#define DO_CHECK (0)

WORD dynaOpBeqSamePage(BYTE *cp,BYTE op0,BYTE op1,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;

    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    BYTE *intbypass;
    WORD intbypasslen;
    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);

        l+=CMP_RBANK_WITH_IMM(cp+l,242*8,0x80000180);
        intbypass=cp+l;
        intbypasslen=l;
        l+=JNE_SHORT(cp+l,0);
        l+=RETURN(cp+l);
        JNE_SHORT(intbypass,(l-intbypasslen)-2);
    }

    l+=INC_PC_COUNTER(cp+l);

    l+=dynaLoadReg(cp+l,op0);
    l+=CMP_DWORD_WITH_REG(cp+l,dynaMapToNative[op0].Reg,op1*8);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JNE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JNE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBneSamePage(BYTE *cp,BYTE op0,BYTE op1,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;

    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    BYTE *intbypass;
    WORD intbypasslen;
    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);

        l+=CMP_RBANK_WITH_IMM(cp+l,242*8,0x80000180);
        intbypass=cp+l;
        intbypasslen=l;
        l+=JNE_SHORT(cp+l,0);
        l+=RETURN(cp+l);
        JNE_SHORT(intbypass,(l-intbypasslen)-2);
    }

    l+=INC_PC_COUNTER(cp+l);

    l+=dynaLoadReg(cp+l,op0);
    l+=CMP_DWORD_WITH_REG(cp+l,dynaMapToNative[op0].Reg,op1*8);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBlezSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;

    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    BYTE *intbypass;
    WORD intbypasslen;
    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);

        l+=CMP_RBANK_WITH_IMM(cp+l,242*8,0x80000180);
        intbypass=cp+l;
        intbypasslen=l;
        l+=JNE_SHORT(cp+l,0);
        l+=RETURN(cp+l);
        JNE_SHORT(intbypass,(l-intbypasslen)-2);
    }

    l+=INC_PC_COUNTER(cp+l);

    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JG_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JG_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBgtzSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;

    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    BYTE *intbypass;
    WORD intbypasslen;
    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);

        l+=CMP_RBANK_WITH_IMM(cp+l,242*8,0x80000180);
        intbypass=cp+l;
        intbypasslen=l;
        l+=JNE_SHORT(cp+l,0);
        l+=RETURN(cp+l);
        JNE_SHORT(intbypass,(l-intbypasslen)-2);
    }

    l+=INC_PC_COUNTER(cp+l);

    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JLE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JLE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBltzSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;

    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    BYTE *intbypass;
    WORD intbypasslen;
    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);

        l+=CMP_RBANK_WITH_IMM(cp+l,242*8,0x80000180);
        intbypass=cp+l;
        intbypasslen=l;
        l+=JNE_SHORT(cp+l,0);
        l+=RETURN(cp+l);
        JNE_SHORT(intbypass,(l-intbypasslen)-2);
    }

    l+=INC_PC_COUNTER(cp+l);

    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JGE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JGE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBgezSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;

    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    BYTE *intbypass;
    WORD intbypasslen;
    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);

        l+=CMP_RBANK_WITH_IMM(cp+l,242*8,0x80000180);
        intbypass=cp+l;
        intbypasslen=l;
        l+=JNE_SHORT(cp+l,0);
        l+=RETURN(cp+l);
        JNE_SHORT(intbypass,(l-intbypasslen)-2);
    }

    l+=INC_PC_COUNTER(cp+l);

    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JL_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JL_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

// --- Likely Branches ---
WORD dynaOpBeqlSamePage(BYTE *cp,BYTE op0,BYTE op1,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=dynaLoadReg(cp+l,op0);
    l+=CMP_DWORD_WITH_REG(cp+l,dynaMapToNative[op0].Reg,op1*8);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JNE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JNE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBnelSamePage(BYTE *cp,BYTE op0,BYTE op1,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=dynaLoadReg(cp+l,op0);
    l+=CMP_DWORD_WITH_REG(cp+l,dynaMapToNative[op0].Reg,op1*8);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBlezlSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JG_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JG_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBgtzlSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JLE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JLE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBltzlSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JGE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JGE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBgezlSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JL_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JL_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

// --- Branch with link ---
WORD dynaOpBltzalSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=STORE_DWORD_TO_RBANK(cp+l,dynaMapToNative[31].Reg,dynaPC); // $ra
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JGE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JGE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBgezalSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=STORE_DWORD_TO_RBANK(cp+l,dynaMapToNative[31].Reg,dynaPC); // $ra
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JL_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JL_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBltzallSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=STORE_DWORD_TO_RBANK(cp+l,dynaMapToNative[31].Reg,dynaPC); // $ra
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JGE_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JGE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

WORD dynaOpBgezallSamePage(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=STORE_DWORD_TO_RBANK(cp+l,dynaMapToNative[31].Reg,dynaPC); // $ra
    l+=CMP_RBANK_WITH_IMM_BYTE(cp+l,op0*8,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JL_LONG(cp+l,0);

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);

    JL_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}
// --- Unconditional Jumps ---
WORD dynaOpJSamePage(BYTE *cp,DWORD NewPC)
{
    WORD l=0;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=JMP_LONG(cp+l,CompiledPC);
    return(l);
}

WORD dynaOpJALSamePage(BYTE *cp,DWORD NewPC)
{
    WORD l=0;
    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=STORE_DWORD_TO_RBANK(cp+l,dynaMapToNative[31].Reg,dynaPC); // $ra
    l+=JMP_LONG(cp+l,CompiledPC);
    return(l);
}

// --- Register-based Jumps ---
WORD dynaOpJRSamePage(BYTE *cp,BYTE op0)
{
    WORD l=0;
    DWORD CompiledPC;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=dynaLoadReg(cp+l,op0);           // Load value of rs
    l+=MOV_REG_TO_TMP(cp+l,dynaMapToNative[op0].Reg); // Move to temporary ARM64 reg
    CompiledPC=dynaGetCompiledAddress_TMP();            // Convert tmp to compiled address
    l+=JMP_LONG(cp+l,CompiledPC);
    return(l);
}

WORD dynaOpJALRSamePage(BYTE *cp,BYTE op0)
{
    WORD l=0;
    DWORD CompiledPC;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=STORE_DWORD_TO_RBANK(cp+l,dynaMapToNative[31].Reg,dynaPC); // $ra
    l+=dynaLoadReg(cp+l,op0);           // Load value of rs
    l+=MOV_REG_TO_TMP(cp+l,dynaMapToNative[op0].Reg); // Move to temporary ARM64 reg
    CompiledPC=dynaGetCompiledAddress_TMP();            // Convert tmp to compiled address
    l+=JMP_LONG(cp+l,CompiledPC);
    return(l);
}
// --- Branch Likely Variants ---
WORD dynaOpBeqlSamePage_Likely(BYTE *cp,BYTE op0,BYTE op1,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr,*fixup2Ptr;
    WORD fixup1Len,fixup2Len;

    DWORD TruePC=dynaGetCompiledAddress(NewPC);
    DWORD FalsePC=dynaGetCompiledAddress(dynaPC+8);

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=INC_PC_COUNTER_S(cp+l);

    l+=dynaLoadReg(cp+l,op0);
    l+=CMP_DWORD_WITH_REG(cp+l,dynaMapToNative[op0].Reg,op1*8);
    DWORD Offset=FalsePC-(DWORD)(cp+l+6);
    l+=JNE_LONG(cp+l,Offset); // Skip next instruction if not equal

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,TruePC);
    return(l);
}

WORD dynaOpBnelSamePage_Likely(BYTE *cp,BYTE op0,BYTE op1,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr,*fixup2Ptr;
    WORD fixup1Len,fixup2Len;

    DWORD TruePC=dynaGetCompiledAddress(NewPC);
    DWORD FalsePC=dynaGetCompiledAddress(dynaPC+8);

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);
    l+=INC_PC_COUNTER_S(cp+l);

    l+=dynaLoadReg(cp+l,op0);
    l+=CMP_DWORD_WITH_REG(cp+l,dynaMapToNative[op0].Reg,op1*8);
    DWORD Offset=FalsePC-(DWORD)(cp+l+6);
    l+=JE_LONG(cp+l,Offset); // Skip next instruction if equal

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=JMP_LONG(cp+l,TruePC);
    return(l);
}

// --- Branch with Link Variants ---
WORD dynaOpBltzalSamePage_Likely(BYTE *cp,BYTE op0,DWORD NewPC)
{
    WORD l=0;
    BYTE *fixup1Ptr;
    WORD fixup1Len;

    DWORD CompiledPC=dynaGetCompiledAddress(NewPC);
    NewPC |=0x80000000;

    if(DO_CHECK)
    {
        l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC,242*8);
        l+=CALL_CHECKINTS(cp+l);
    }

    l+=INC_PC_COUNTER(cp+l);

    l+=CMP_RBANK_WITH_IMM(cp+l,op0*8+4,0);
    fixup1Ptr=cp+l;
    fixup1Len=l;
    l+=JGE_LONG(cp+l,0); // branch if >= 0

    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    l+=STORE_DWORD_TO_RBANK(cp+l,dynaPC+8,31*8); // link address
    l+=JMP_LONG(cp+l,CompiledPC);

    JGE_LONG(fixup1Ptr,(l-fixup1Len)-6);
    return(l);
}

// --- Delay Slot Handler (for ARM64 port) ---
WORD dynaOpDelaySlotHandler(BYTE *cp)
{
    WORD l=0;
    // In ARM64, we simply execute the instruction in delay slot immediately
    // This ensures MIPS semantics are preserved
    iOpCode=dynaNextOpCode;
    l+=MainInstruction[dynaNextOpCode>>26](cp+l);
    return(l);
}
