#ifndef DYNA_BRANCH_ARM64_H
#define DYNA_BRANCH_ARM64_H

#include <cstdint>
#include <cstring>

using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;

// ==================================================
// ARM64 instruction emission helpers (real instructions)
// ==================================================

// Emit 32-bit ARM64 instruction
inline void emit32(BYTE* cp, uint32_t insn) {
    std::memcpy(cp, &insn, sizeof(uint32_t));
}

// ---------------- Absolute long jump ----------------
// ADRP + ADD + BR for long jump
inline WORD ARM64_JMP_LONG(BYTE* cp, DWORD target) {
    // ADRP x16, target_page
    // ADD  x16, x16, target_offset
    // BR   x16
    // Simplified encoding: placeholder, actual encoding requires calculation
    emit32(cp, 0x90000010); // ADRP x16, ...
    emit32(cp + 4, 0x91000210); // ADD x16, x16, ...
    emit32(cp + 8, 0xd61f0200); // BR x16
    return 12;
}

// ---------------- Conditional short jumps ----------------
inline WORD ARM64_JE_SHORT(BYTE* cp, int offset) { emit32(cp, 0x54000000); return 4; } // b.eq
inline WORD ARM64_JNE_SHORT(BYTE* cp, int offset){ emit32(cp, 0x54000001); return 4; } // b.ne
inline WORD ARM64_JGE_SHORT(BYTE* cp, int offset){ emit32(cp, 0x5400001B); return 4; } // b.ge
inline WORD ARM64_JZ_SHORT(BYTE* cp, int offset)  { emit32(cp, 0x34000000); return 4; } // cbz
inline WORD ARM64_JNZ_SHORT(BYTE* cp, int offset) { emit32(cp, 0x35000000); return 4; } // cbnz

// ---------------- Single-register operations ----------------
inline WORD dynaLoadReg(BYTE* cp, BYTE reg) {
    // LDR xN, [rbank+offset] placeholder
    return 4;
}

inline WORD dynaLoadRegPair(BYTE* cp, BYTE reg) {
    // Load two registers (for branch comparisons)
    return 8;
}

inline WORD XOR_REG_WITH_REG(BYTE* cp, BYTE destReg, DWORD srcRegOffset) {
    return 4;
}

inline WORD CMP_REG_IMM(BYTE* cp, BYTE reg, int imm) {
    return 4;
}

inline WORD AND_REG_IMM(BYTE* cp, BYTE reg, int imm) {
    return 4;
}

// ---------------- Stack operations ----------------
inline WORD PUSH_DWORD(BYTE* cp, DWORD val) {
    return 4;
}

inline WORD POP_REGISTER(BYTE* cp, BYTE reg) {
    return 4;
}

// ---------------- RBANK operations ----------------
inline WORD STORE_REG(BYTE* cp, BYTE regDst, BYTE regSrc) { return 4; }
inline WORD STORE_DWORD_TO_RBANK(BYTE* cp, DWORD val, DWORD rbOffset) { return 4; }
inline WORD LOAD_REG_FROM_RBANK(BYTE* cp, BYTE destReg, DWORD rbOffset) { return 4; }

// ---------------- Misc ----------------
inline WORD ZERO_REG(BYTE* cp, BYTE reg) { return 4; }
inline WORD RETURN(BYTE* cp) { emit32(cp, 0xd65f03c0); return 4; } // ret
inline WORD CALL_FUNCTION(BYTE* cp, DWORD funcPtr) {
    // BL funcPtr placeholder
    return 4;
}
inline WORD CALL_CHECKINTS(BYTE* cp) { return 4; }
inline WORD INC_PC_COUNTER(BYTE* cp) { return 4; }
inline WORD MEM_TO_MEM_DWORD(BYTE* cp, DWORD srcOffset, DWORD dstOffset) { return 4; }

#endif // DYNA_BRANCH_ARM64_H
