#ifndef DYNA_NATIVE_H
#define DYNA_NATIVE_H

#include <cstdint>
#include <cstddef>

using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;
using QWORD = uint64_t;
using CodePtr = uint8_t*;

// General purpose
extern WORD PUSH_STACK_POINTER(CodePtr cp);
extern WORD POP_STACK_POINTER(CodePtr cp);
extern WORD PUSH_REGISTER(CodePtr cp, BYTE Reg);
extern WORD POP_REGISTER(CodePtr cp);
extern WORD LOAD_REG_DWORD(CodePtr cp, BYTE Reg, DWORD Value);
extern WORD LOAD_REG_QWORD(CodePtr cp, BYTE Reg, QWORD Value);
extern WORD STORE_REG_TO_MEM(CodePtr cp, BYTE Reg, DWORD Address);
extern WORD LOAD_REG_FROM_MEM(CodePtr cp, BYTE Reg, DWORD Address);
extern WORD MOV_NATIVE_REG_REG(CodePtr cp, BYTE dst, BYTE src);

// Arithmetic
extern WORD ADD_REG_IMM(CodePtr cp, BYTE Reg, DWORD Imm);
extern WORD SUB_REG_IMM(CodePtr cp, BYTE Reg, DWORD Imm);
extern WORD ADD_REG_REG(CodePtr cp, BYTE dst, BYTE src);
extern WORD SUB_REG_REG(CodePtr cp, BYTE dst, BYTE src);
extern WORD MUL_REG_REG(CodePtr cp, BYTE dst, BYTE src);
extern WORD DIV_REG_REG(CodePtr cp, BYTE dst, BYTE src);

// Logic
extern WORD AND_REG_IMM(CodePtr cp, BYTE Reg, DWORD Imm);
extern WORD OR_REG_IMM(CodePtr cp, BYTE Reg, DWORD Imm);
extern WORD XOR_REG_IMM(CodePtr cp, BYTE Reg, DWORD Imm);
extern WORD AND_REG_REG(CodePtr cp, BYTE dst, BYTE src);
extern WORD OR_REG_REG(CodePtr cp, BYTE dst, BYTE src);
extern WORD XOR_REG_REG(CodePtr cp, BYTE dst, BYTE src);

// Branching
extern WORD CMP_REG_IMM(CodePtr cp, BYTE Reg, DWORD Imm);
extern WORD CMP_REG_REG(CodePtr cp, BYTE Reg1, BYTE Reg2);
extern WORD B_EQ(CodePtr cp, int32_t offset);
extern WORD B_NE(CodePtr cp, int32_t offset);
extern WORD B_LT(CodePtr cp, int32_t offset);
extern WORD B_GT(CodePtr cp, int32_t offset);
extern WORD B_LE(CodePtr cp, int32_t offset);
extern WORD B_GE(CodePtr cp, int32_t offset);
extern WORD BR_REGISTER(CodePtr cp, BYTE Reg);
extern WORD CALL_REGISTER(CodePtr cp, BYTE Reg);
extern WORD RETURN(CodePtr cp);

// Memory copy
extern WORD MEM_TO_MEM_DWORD(CodePtr cp, DWORD DstOff, DWORD SrcOff);
extern WORD MEM_TO_MEM_QWORD(CodePtr cp, DWORD DstOff, DWORD SrcOff);

// Misc
extern WORD NOP(CodePtr cp);

#endif // DYNA_NATIVE_H
