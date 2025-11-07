#ifndef DYNA_MEMORY
#define DYNA_MEMORY

#include <switch.h>
#include <cstdint>
#include <cstring>

using BYTE = uint8_t;
using WORD = uint16_t;
using DWORD = uint32_t;

// --------------------------------------------------------
// External RAM / code cache
// --------------------------------------------------------
extern uint8_t* dynaRamPtr;
extern uint8_t* codeCachePtr;

// --------------------------------------------------------
// Integer registers / constants
// --------------------------------------------------------
#define MEM_PTR 0
#define NATIVE_0 1
#define NATIVE_1 2
#define NATIVE_2 3

#define _FPR 100
#define CPR1_ 200
#define MEM_MASK 0x1FFFFF

// --------------------------------------------------------
// Low-level ARM64 instruction helpers
// --------------------------------------------------------
inline WORD INC_PC_COUNTER(BYTE* cp) { return 0; }

inline WORD LOAD_REG(BYTE* cp, BYTE src, BYTE dst) {
    *(uint32_t*)cp = 0xF9400000 | (dst << 0) | (src << 5);
    return 4;
}

inline WORD STORE_REG_TO_RBANK(BYTE* cp, BYTE src, DWORD offset) {
    *(uint32_t*)cp = 0x58000050;
    *(uint64_t*)(cp+4) = offset;
    *(uint32_t*)(cp+12) = 0xD61F0200;
    return 16;
}

inline WORD LOAD_REG_FROM_RBANK(BYTE* cp, BYTE dst, DWORD offset) {
    *(uint32_t*)cp = 0x58000050;
    *(uint64_t*)(cp+4) = offset;
    *(uint32_t*)(cp+12) = 0xD61F0200;
    return 16;
}

inline WORD ADD_REG_IMM(BYTE* cp, BYTE reg, DWORD imm) {
    *(uint32_t*)cp = 0x91000000 | (reg << 0) | (reg << 5);
    return 4;
}

inline WORD AND_REG_IMM(BYTE* cp, BYTE reg, DWORD imm) {
    *(uint32_t*)cp = 0x92000000 | (reg << 0) | (reg << 5);
    return 4;
}

inline WORD LOAD_MEM_REG(BYTE* cp, BYTE addrReg, BYTE dstReg) {
    *(uint32_t*)cp = 0xF9400000 | (dstReg << 0) | (addrReg << 5);
    return 4;
}

inline WORD STORE_MEM_REG(BYTE* cp, BYTE addrReg, BYTE srcReg) {
    *(uint32_t*)cp = 0xF9000000 | (srcReg << 0) | (addrReg << 5);
    return 4;
}

// --------------------------------------------------------
// Stack helpers
// --------------------------------------------------------
inline WORD PUSH_REGISTER(BYTE* cp, BYTE reg) {
    *(uint32_t*)cp = 0xD10043FF; // SUB SP, SP, #16
    *(uint32_t*)(cp+4) = 0xF9000000 | reg; // STR Xreg, [SP,#0]
    return 8;
}

inline WORD POP_REGISTER(BYTE* cp, BYTE reg) {
    *(uint32_t*)cp = 0xF9400000 | reg; // LDR Xreg, [SP,#0]
    *(uint32_t*)(cp+4) = 0x910043FF;    // ADD SP, SP, #16
    return 8;
}

// --------------------------------------------------------
// CALL/branch helpers
// --------------------------------------------------------
inline WORD EMIT_CALL(BYTE* cp, void* target) {
    *(uint32_t*)cp = 0x58000050;
    *(uint64_t*)(cp+4) = (uint64_t)target;
    *(uint32_t*)(cp+12) = 0xD61F0200;
    return 16;
}

inline WORD EMIT_BRANCH(BYTE* cp, DWORD src, DWORD dst) {
    int64_t offset = ((int64_t)dst - (int64_t)src) >> 2;
    *(uint32_t*)cp = 0x14000000 | (offset & 0x3FFFFFF);
    return 4;
}

inline WORD EMIT_COND_BRANCH(BYTE* cp, BYTE cond, DWORD src, DWORD dst) {
    int64_t offset = ((int64_t)dst - (int64_t)src) >> 2;
    *(uint32_t*)cp = 0x54000000 | (cond << 0) | (offset & 0x7FFFF);
    return 4;
}

// --------------------------------------------------------
// Generic load/store macros
// --------------------------------------------------------
#define DEFINE_LOAD_OP(name) \
WORD name(BYTE* cp, BYTE op0, BYTE op1, DWORD Imm){ \
    WORD l=0; \
    l+=LOAD_REG(cp+l, op1, MEM_PTR); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, Imm); \
    l+=AND_REG_IMM(cp+l, MEM_PTR, MEM_MASK); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, (DWORD)dynaRamPtr); \
    l+=LOAD_MEM_REG(cp+l, MEM_PTR, NATIVE_0); \
    l+=STORE_REG_TO_RBANK(cp+l, NATIVE_0, op0*4); \
    return l; \
}

#define DEFINE_STORE_OP(name) \
WORD name(BYTE* cp, BYTE op0, BYTE op1, DWORD Imm){ \
    WORD l=0; \
    l+=LOAD_REG(cp+l, op1, MEM_PTR); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, Imm); \
    l+=AND_REG_IMM(cp+l, MEM_PTR, MEM_MASK); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, (DWORD)dynaRamPtr); \
    l+=LOAD_REG_FROM_RBANK(cp+l, NATIVE_0, op0*4); \
    l+=STORE_MEM_REG(cp+l, MEM_PTR, NATIVE_0); \
    return l; \
}

// --------------------------------------------------------
// Floating-point load/store macros
// --------------------------------------------------------
#define DEFINE_FLOAD_OP(name, regbank, size) \
WORD name(BYTE* cp, BYTE op0, BYTE op1, DWORD Imm){ \
    WORD l=0; \
    l+=LOAD_REG(cp+l, op1, MEM_PTR); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, Imm); \
    l+=AND_REG_IMM(cp+l, MEM_PTR, MEM_MASK); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, (DWORD)dynaRamPtr); \
    for(int i=0;i<size;i++){ \
        l+=LOAD_MEM_REG(cp+l, MEM_PTR, NATIVE_0); \
        l+=STORE_REG_TO_RBANK(cp+l, NATIVE_0, regbank+op0*4+i*4); \
        l+=ADD_REG_IMM(cp+l, MEM_PTR, 4); \
    } \
    return l; \
}

#define DEFINE_FSTORE_OP(name, regbank, size) \
WORD name(BYTE* cp, BYTE op0, BYTE op1, DWORD Imm){ \
    WORD l=0; \
    l+=LOAD_REG(cp+l, op1, MEM_PTR); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, Imm); \
    l+=AND_REG_IMM(cp+l, MEM_PTR, MEM_MASK); \
    l+=ADD_REG_IMM(cp+l, MEM_PTR, (DWORD)dynaRamPtr); \
    for(int i=0;i<size;i++){ \
        l+=LOAD_REG_FROM_RBANK(cp+l, NATIVE_0, regbank+op0*4+i*4); \
        l+=STORE_MEM_REG(cp+l, MEM_PTR, NATIVE_0); \
        l+=ADD_REG_IMM(cp+l, MEM_PTR, 4); \
    } \
    return l; \
}

// --------------------------------------------------------
// Generate all integer ops
// --------------------------------------------------------
DEFINE_LOAD_OP(dynaOpLb) DEFINE_LOAD_OP(dynaOpLbU)
DEFINE_LOAD_OP(dynaOpLh) DEFINE_LOAD_OP(dynaOpLhU)
DEFINE_LOAD_OP(dynaOpLw) DEFINE_LOAD_OP(dynaOpLwU)
DEFINE_LOAD_OP(dynaOpLd)

DEFINE_STORE_OP(dynaOpSb) DEFINE_STORE_OP(dynaOpSh)
DEFINE_STORE_OP(dynaOpSw) DEFINE_STORE_OP(dynaOpSd)

// --------------------------------------------------------
// Generate all floating-point ops
// --------------------------------------------------------
DEFINE_FLOAD_OP(dynaOpLwc1, _FPR, 1)
DEFINE_FLOAD_OP(dynaOpLwc2, CPR1_, 2)
DEFINE_FLOAD_OP(dynaOpLdc1, _FPR, 2)
DEFINE_FLOAD_OP(dynaOpLdc2, CPR1_, 2)

DEFINE_FSTORE_OP(dynaOpSwc1, _FPR, 1)
DEFINE_FSTORE_OP(dynaOpSwc2, CPR1_, 2)
DEFINE_FSTORE_OP(dynaOpSdc1, _FPR, 2)
DEFINE_FSTORE_OP(dynaOpSdc2, CPR1_, 2)

// --------------------------------------------------------
// Atomic ops: SC / SCD
// --------------------------------------------------------
WORD dynaOpSc(BYTE* cp, BYTE op0, BYTE op1, DWORD Imm) {
    WORD l = 0;
    l += LOAD_REG(cp + l, op1, MEM_PTR);
    l += ADD_REG_IMM(cp + l, MEM_PTR, Imm);
    l += AND_REG_IMM(cp + l, MEM_PTR, MEM_MASK);
    l += ADD_REG_IMM(cp + l, MEM_PTR, (DWORD)dynaRamPtr);
    l += LOAD_REG_FROM_RBANK(cp + l, NATIVE_0, op0*4);
    *(uint32_t*)(cp + l) = 0x88000000 | (NATIVE_0 << 0) | (MEM_PTR << 5) | (NATIVE_1 << 16); l += 4;
    *(uint32_t*)(cp + l) = 0xD2800000 | (0 << 0) | (NATIVE_1 << 5); l += 4;
    l += STORE_REG_TO_RBANK(cp + l, NATIVE_1, op0*4);
    return l;
}

WORD dynaOpScd(BYTE* cp, BYTE op0, BYTE op1, DWORD Imm) {
    WORD l = 0;
    l += LOAD_REG(cp + l, op1, MEM_PTR);
    l += ADD_REG_IMM(cp + l, MEM_PTR, Imm);
    l += AND_REG_IMM(cp + l, MEM_PTR, MEM_MASK);
    l += ADD_REG_IMM(cp + l, MEM_PTR, (DWORD)dynaRamPtr);
    l += LOAD_REG_FROM_RBANK(cp + l, NATIVE_0, op0*8);
    *(uint32_t*)(cp + l) = 0x88000000 | (NATIVE_0 << 0) | (MEM_PTR << 5) | (NATIVE_1 << 16); l += 4;
    *(uint32_t*)(cp + l) = 0xD2800000 | (0 << 0) | (NATIVE_1 << 5); l += 4;
    l += STORE_REG_TO_RBANK(cp + l, NATIVE_1, op0*8);
    return l;
}

// --------------------------------------------------------
// Simple MIPS -> ARM64 dynamic recompilation loop
// --------------------------------------------------------
void RecompileMIPS(uint8_t* mipsCode, size_t size) {
    BYTE* cp = codeCachePtr;
    for(size_t pc=0; pc<size; pc+=4) {
        DWORD instr = *(DWORD*)(mipsCode+pc);
        DWORD opcode = (instr>>26)&0x3F;

        switch(opcode) {
            case 0x00: { // SPECIAL
                DWORD func = instr&0x3F;
                switch(func) {
                    case 0x08: cp += EMIT_BRANCH(cp, (DWORD)cp, 0); break; // JR
                    case 0x09: cp += EMIT_CALL(cp, nullptr); break;       // JALR
                    default: cp += 16; break;
                }
            } break;

            case 0x02: cp += EMIT_BRANCH(cp, (DWORD)cp, (instr&0x03FFFFFF)<<2); break;
            case 0x03: cp += EMIT_CALL(cp, (void*)((instr&0x03FFFFFF)<<2)); break;
            case 0x04: cp += EMIT_COND_BRANCH(cp, 0, (DWORD)cp, (DWORD)cp+((int16_t)(instr&0xFFFF)<<2)); break;
            case 0x05: cp += EMIT_COND_BRANCH(cp, 1, (DWORD)cp, (DWORD)cp+((int16_t)(instr&0xFFFF)<<2)); break;

            default: cp += 16; break;
        }
    }
}

#endif // DYNA_MEMORY
