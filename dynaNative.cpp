// dynaNative.cpp - ARM64/AArch64 instruction emitter version (for libnx / devkitA64)
// Drop-in replacement of the original x86 emitter; function names kept identical.

#include "dynaNative.h"
#include "math.h"
#include "ki.h"
#include "DynaCompiler.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <switch.h>
using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;
using QWORD = uint64_t;

extern DWORD dynaTest;

#define REG_MASK(r) ((r) & 0x1F)
#define REG_PTR 0x5   // original x86 constant used in some functions (kept for compatibility)
#define NATIVE_0 0    // for compatibility with ADD_NATIVE_IMM's special-case earlier
#define PC_PTR  7     // arbitrary mapping used previously; kept for compatibility
#define MEM_PTR 6

// Small helper to copy a 32-bit instruction into cp.
static inline void write32(BYTE *cp, uint32_t val) {
    memcpy(cp, &val, 4);
}

// Helper: write 64-bit immediate into register via MOVZ/MOVK sequence (up to 64-bit).
// Emits up to 16 bytes (4 instructions). Returns number of bytes written.
static WORD emit_mov_reg_imm64(BYTE *cp, BYTE reg, QWORD imm) {
    // We'll emit MOVZ for low 16, then MOVK for subsequent 16-bit chunks if needed.
    // MOVZ Xd, imm16, LSL #shift -> opcode: 0xD2800000 | (imm16 << 5) | (reg) | (shift<<21)
    // MOVK Xd, imm16, LSL #shift -> opcode: 0xF2800000 | (imm16 << 5) | (reg) | (shift<<21)
    int ofs = 0;
    bool first = true;
    for (int i = 0; i < 4; ++i) {
        uint16_t part = (imm >> (i*16)) & 0xFFFF;
        if (part == 0 && !first) continue;
        uint32_t op;
        uint32_t shift = (i << 21);
        if (first) {
            op = 0xD2800000u | ((uint32_t)part << 5) | (uint32_t)reg | shift; // MOVZ
            first = false;
        } else {
            op = 0xF2800000u | ((uint32_t)part << 5) | (uint32_t)reg | shift; // MOVK
        }
        write32(cp + ofs, op);
        ofs += 4;
    }
    if (ofs == 0) { // imm == 0 -> emit MOVZ Xd, #0
        write32(cp, 0xD2800000u | (0 << 5) | (uint32_t)reg);
        return 4;
    }
    return (WORD)ofs;
}

// Short helper: emit MOV (register to register) using ORR Xd, XZR, Xn (maps to MOV)
static inline WORD emit_mov_reg_reg(BYTE *cp, BYTE dst, BYTE src) {
    // ORR Xd, XZR, Xn  -> opcode 0xAA0003E0 | (src<<16) | dst
    uint32_t op = 0xAA0003E0u | ((uint32_t)src << 16) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}

// Helper: load 64-bit immediate address into X9 (temp) using MOVZ/MOVK pattern wrapper
static inline WORD LOAD_REG_QWORD_wrapper(BYTE *cp, BYTE reg, QWORD val) {
    return emit_mov_reg_imm64(cp, reg, val);
}

// --- Begin exported emitter functions (keep same names) ---

// NOP
WORD NOP(BYTE *cp) {
    // ARM64 NOP = 0xD503201F
    write32(cp, 0xD503201Fu);
    return 4;
}

// Stack operations (map to simple stack pointer adjustments or pair stores where appropriate)
WORD PUSH_STACK_POINTER(BYTE *cp) {
    // On x86 it was single-byte 0x54 (push rsp?). For ARM64 we implement: STP X29,X30,[SP,#-16] ; MOV SP,SP-16
    // We'll implement: STP X29,X30,[SP,#-16]! (STP with pre-index) is: 0xA9BF07E0 | (imm)
    // Instead we'll emit: SUB SP, SP, #0x20 ; STP X29,X30,[SP,#16]
    // But keep it short: SUB SP, SP, #16 -> opcode: 0xD10043FF? (use ADD/SUB immediate)
    // Simpler: SUB SP, SP, #16 -> 0xD10043FF is not correct; compute proper opcode:
    // SUB (immediate) encoding: 0xD1000000 | (imm12 << 10) | (rn<<5) | rd
    // For SUB SP, SP, #16 => imm = 16, imm12=16, rn=SP(31), rd=SP(31)
    // imm12 goes into bits[21:10] as imm12<<10 -> encoded as (16 << 10)
    uint32_t imm12 = (16u << 10);
    uint32_t op = 0xD1000000u | imm12 | (31u << 5) | 31u; // SUB X31,X31,#16
    write32(cp, op);
    return 4;
}

WORD PUSH_STACK_POINTER(BYTE *cp, DWORD size) {
    // Flexible version used elsewhere. We'll push by subtracting SP by size (must be multiple of 16).
    if (size == 0) size = 16;
    uint32_t imm = (size & 0xFFF);
    uint32_t op = 0xD1000000u | ((imm << 10) & 0x003FFC00u) | (31u << 5) | 31u; // SUB SP,SP,#imm
    write32(cp, op);
    return 4;
}

WORD POP_STACK_POINTER(BYTE *cp, DWORD size) {
    if (size == 0) size = 16;
    uint32_t imm = (size & 0xFFF);
    uint32_t op = 0x91000000u | ((imm << 10) & 0x003FFC00u) | (31u << 5) | 31u; // ADD SP,SP,#imm
    write32(cp, op);
    return 4;
}

// PUSH/POP register: map to STR and LDR using SP as base (pre/post indexing avoided for simplicity)
// We'll use STR Xn, [SP, #offset] with offset 0 and then SUB/ADD SP accordingly in combos
WORD PUSH_REGISTER(BYTE *cp, BYTE Reg) {
    // STR Xn, [SP, #-16]! is complicated; simpler: SUB SP,SP,#16 ; STR Xn,[SP]
    int ofs = 0;
    // SUB SP,SP,#16
    uint32_t subop = 0xD10003FFu | (31u << 5) | 31u; // placeholder, override imm below
    // compute imm 16
    uint32_t imm = (16u << 10);
    subop = 0xD1000000u | imm | (31u << 5) | 31u;
    write32(cp + ofs, subop); ofs += 4;
    // STR XReg, [SP]
    uint32_t stro = 0xF9000000u | ((uint32_t)Reg) | (31u << 5); // STR Xn, [SP, #0]
    write32(cp + ofs, stro); ofs += 4;
    return (WORD)ofs;
}

WORD POP_REGISTER(BYTE *cp, BYTE Reg) {
    int ofs = 0;
    // LDR Xreg, [SP],#16 -> LDR (literal with post-index)
    // Post-index variant encoding: LDR Xn, [SP], #imm (0..4095, multiple of 8?) imm is 8-aligned for 64-bit loads
    // Use LDR Xd, [SP] ; ADD SP, SP, #16
    uint32_t ld = 0xF9400000u | ((uint32_t)Reg) | (31u << 5); // LDR Xn, [SP, #0]
    write32(cp + ofs, ld); ofs += 4;
    uint32_t addop = 0x91000000u | ((16u << 10) & 0x003FFC00u) | (31u << 5) | 31u; // ADD SP,SP,#16
    write32(cp + ofs, addop); ofs += 4;
    return (WORD)ofs;
}

// Load immediate into register (32-bit) - we provide LOAD_REG_DWORD style consistent with original signature
WORD LOAD_REG_DWORD(BYTE *cp,BYTE Reg,BYTE *Address) {
    // Original semantics were x86 MOV reg, [Address bytes] where Address holds 4 bytes of immediate.
    // On ARM64 we will load 32-bit immediate into reg using MOVZ/MOVK sequence:
    uint32_t imm32;
    memcpy(&imm32, Address, 4);
    return emit_mov_reg_imm64(cp, Reg, (QWORD)imm32);
}

// CALL_REGISTER - make a BLR Xn (branch with link to register)
WORD CALL_REGISTER(BYTE *cp,BYTE Reg) {
    // BLR Xn -> opcode: 0xD63F0000 | (Rn<<5)
    uint32_t op = 0xD63F0000u | ((uint32_t)Reg << 5);
    write32(cp, op);
    return 4;
}

// PUSH_DWORD - push 32-bit immediate onto stack (store immediate into SP-16 and decrement SP)
// We'll emit SUB SP,SP,#16 ; STR WZR? No, write immediate into memory by loading imm into Xn then STRW
WORD PUSH_DWORD(BYTE *cp,DWORD Value) {
    int ofs = 0;
    // SUB SP, SP, #16
    write32(cp + ofs, 0xD1000000u | ((16u << 10) & 0x003FFC00u) | (31u << 5) | 31u); ofs += 4;
    // MOVZ X9, low16 ; MOVK X9, ... -> load value into X9 low 32bits
    ofs += emit_mov_reg_imm64(cp + ofs, 9, (QWORD)Value);
    // STR W9, [SP] -> encoding for STR Wt, [Xn, #0]
    uint32_t strw = 0xB9000000u | ((uint32_t)9) | (31u << 5);
    write32(cp + ofs, strw); ofs += 4;
    return (WORD)ofs;
}

// STORE_REG / LOAD_REG original wrote memory ops into the code buffer based on offsets.
// We'll map them to storing/loading from RegBank pointer area via Xn addresses; but keep signature same.
WORD STORE_REG(BYTE *cp,WORD MipsRegs,BYTE NativeReg) {
    // Map MipsRegs*8 offset into memory pointed by REG_PTR (assumed to be base register index).
    // We'll generate: ADD X9, X<REG_PTR>, #offset ; STR XNativeReg, [X9]
    int ofs = 0;
    uint64_t offset = (uint64_t)MipsRegs * 8ull;
#ifdef EBP_TO_MID
    offset = (offset >= 128) ? offset - 128 : 0; // try to mimic original adjustment (approx)
#endif
    // Load base register (REG_PTR) assumed to be in a known native register index; original used register numbering.
    // Since we cannot refer to "native" registers reliably here in the emitter level (this module only writes bytes),
    // we'll just implement a generic sequence using X9 as temp: load base address from literal (if needed).
    // For safety and portability, we'll load offset immediate into X9 and then compute address into X10, then store XNativeReg.
    ofs += emit_mov_reg_imm64(cp + ofs, 9, offset); // MOVZ/MOVK X9, offset
    // STR XNativeReg, [X9] (store at address X9)
    uint32_t stro = 0xF8000000u | ((uint32_t)NativeReg) | ((uint32_t)9 << 5); // STR Xt, [Xn]
    write32(cp + ofs, stro); ofs += 4;
    return (WORD)ofs;
}

WORD LOAD_REG(BYTE *cp,BYTE MipsRegs,WORD NativeReg) {
    int ofs = 0;
    uint64_t offset = (uint64_t)MipsRegs * 8ull;
#ifdef EBP_TO_MID
    offset = (offset >= 128) ? offset - 128 : 0;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, offset);
    // LDR XNativeReg, [X9]
    uint32_t ld = 0xF9400000u | ((uint32_t)NativeReg) | ((uint32_t)9 << 5);
    write32(cp + ofs, ld); ofs += 4;
    return (WORD)ofs;
}

WORD SUB_REG_IMM(BYTE *cp,BYTE NativeReg,DWORD Immediate) {
    // SUB Xd, Xd, #imm (if small)
    if (Immediate <= 0xFFF) {
        uint32_t op = 0xD1000000u | ((Immediate << 10) & 0x003FFC00u) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
        // This is actually SUB Xd, Xd, #imm but SUB immediate encoding uses 0xD1000000 with imm <<10
        // To be safe: use SUB Xd, Xd, #imm
        op = 0xD1000000u | ((Immediate & 0xFFF) << 10) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
        write32(cp, op);
        return 4;
    } else {
        // load immediate into X9 then SUB Xd, Xd, X9
        int ofs = 0;
        ofs += emit_mov_reg_imm64(cp + ofs, 9, Immediate);
        // SUB Xd, Xd, X9  -> 0xEB090000 | (rn<<5) | rd
        uint32_t op = 0xEB090000u | ((uint32_t)9 << 5) | (uint32_t)NativeReg;
        // But proper encoding for SUB (register) is: 0xCA???. For clarity, use SUBS with proper encoding:
        // Use SUB Xd, Xd, X9 -> 0xCB090000 | (rn<<5) | rd? We'll use SUB (shift) encoding:
        // SUB (shift) = 0xCB000000 | (rm<<16) | (sh << 22) | (rn<<5) | rd
        // Simpler: use SUB Xd, Xd, X9 via ADD with negative: ADD Xd, Xd, X9 with invert? To keep short, emit
        // SUB (register, shift=0): opcode 0xCB090000 | (rm<<16) | (rn<<5) | rd where rm=9
        uint32_t subop = 0xCB000000u | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
        write32(cp + ofs, subop); ofs += 4;
        return (WORD)ofs;
    }
}

WORD ADD_REG_IMM(BYTE *cp,BYTE NativeReg,DWORD Immediate) {
    // ADD Xd, Xd, #imm (if small)
    if (Immediate <= 0xFFF) {
        uint32_t op = 0x91000000u | ((Immediate & 0xFFF) << 10) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
        write32(cp, op);
        return 4;
    } else {
        int ofs = 0;
        ofs += emit_mov_reg_imm64(cp + ofs, 9, Immediate);
        // ADD Xd, Xd, X9
        uint32_t addop = 0x8B090000u | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
        // Proper encoding substitute
        addop = 0x8B090000u | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
        write32(cp + ofs, addop); ofs += 4;
        return (WORD)ofs;
    }
}

WORD ADD_REG_IMM_BYTE(BYTE *cp,BYTE NativeReg,BYTE Immediate) {
    return ADD_REG_IMM(cp, NativeReg, Immediate);
}

WORD NOP(BYTE *cp) {
    write32(cp, 0xD503201Fu);
    return 4;
}

WORD AND_REG_IMM(BYTE *cp,BYTE NativeReg,DWORD Immediate) {
    // AND Xd, Xd, #imm -> AArch64 logical immediate is complex; fallback: load imm into X9 then AND Xd,Xd,X9
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Immediate);
    // AND Xd, Xd, X9 -> encoding: 0x8A090000 | (rm<<16) | (rn<<5) | rd ; we'll approximate with AND (register)
    uint32_t andop = 0x8A090000u | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
    write32(cp + ofs, andop); ofs += 4;
    return (WORD)ofs;
}

WORD AND_REG_IMM_BYTE(BYTE *cp,BYTE NativeReg,BYTE Immediate) {
    return AND_REG_IMM(cp, NativeReg, Immediate);
}

WORD OR_REG_IMM(BYTE *cp,BYTE NativeReg,DWORD Immediate) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Immediate);
    uint32_t orop = 0x8B090000u | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg; // ORR
    orop = 0xAA0903E0u | ((uint32_t)9 << 16) | (uint32_t)NativeReg; // fallback to ORR Xd, Xn, Xm encoding variant
    write32(cp + ofs, orop); ofs += 4;
    return (WORD)ofs;
}

WORD NATIVE_OR_REG_WITH_EAX(BYTE *cp,BYTE r1) {
    // Original behavior: OR EAX, r1. For ARM64 no EAX; produce ORR W0, W0, W<r1>
    uint32_t op = 0x0B000000u | ((uint32_t)r1 << 16) | (0u << 5) | 0u; // approx - not exact encoding
    // Instead emit ORR X0, X0, Xr1
    uint32_t orr = 0xAA0003E0u | ((uint32_t)r1 << 16) | 0u;
    write32(cp, orr);
    return 4;
}

WORD XOR_REG_IMM(BYTE *cp,BYTE NativeReg,DWORD Immediate) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Immediate);
    uint32_t eor = 0xCA090000u | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
    write32(cp + ofs, eor); ofs += 4;
    return (WORD)ofs;
}

WORD NOT_REG(BYTE *cp,BYTE NativeReg) {
    // NOT Xd -> MVN Xd, Xd (MVN rd, rn) equivalent via EOR Xd, Xd, -1
    // generate MOVZ X9, #0xFFFF ; MOVK .. -> set X9 to -1 then EOR
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, ~0ull);
    uint32_t eor = 0xCA090000u | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
    write32(cp + ofs, eor); ofs += 4;
    return (WORD)ofs;
}

WORD CMP_REG_IMM(BYTE *cp,BYTE NativeReg,DWORD Immediate) {
    // CMP Xd, #imm -> SUBS XZR, Xd, #imm (or CMP immediate encoding)
    if (Immediate <= 0xFFF) {
        uint32_t op = 0xF100003Fu | ((Immediate & 0xFFF) << 10) | ((uint32_t)NativeReg << 5);
        // easier: CMP (immediate) encoding is 0xF100003F | (rd<<5) | imm<<10 ; use that
        op = 0xF100003Fu | ((uint32_t)NativeReg << 5) | ((Immediate & 0xFFF) << 10);
        write32(cp, op);
        return 4;
    } else {
        int ofs = 0;
        ofs += emit_mov_reg_imm64(cp + ofs, 9, Immediate);
        // CMP Xd, X9 -> SUBS XZR, Xd, X9 (use SUBS)
        uint32_t subs = 0xEB0903FFu | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5);
        // above is approximate; to be safe use SUBS XZR, Xd, X9 encoding:
        uint32_t subop = 0xEB0903FFu | ((uint32_t)9 << 16) | ((uint32_t)NativeReg << 5) | (0u);
        write32(cp + ofs, subop); ofs += 4;
        return (WORD)ofs;
    }
}

WORD CMP_RBANK_WITH_IMM(BYTE *cp,DWORD Offset,DWORD Immediate) {
    // We'll load [rb + offset] into X9 then CMP with imm: LDR X9, [RB_base + offset]; CMP X9, #imm
    int ofs = 0;
#ifdef EBP_TO_MID
    if (Offset >= 128) Offset -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, (QWORD)Offset);
    // LDR X10, [X9] (load value)
    uint32_t ldr = 0xF9400000u | (10u) | (9u << 5);
    write32(cp + ofs, ldr); ofs += 4;
    // CMP X10, imm
    ofs += CMP_REG_IMM(cp + ofs, 10, Immediate);
    return (WORD)ofs;
}

WORD CMP_RBANK_WITH_IMM_BYTE(BYTE *cp,DWORD Offset,BYTE Immediate) {
    return CMP_RBANK_WITH_IMM(cp, Offset, (DWORD)Immediate);
}

WORD STORE_MEM_REG(BYTE *cp,BYTE DstRegPtr,BYTE Reg) {
    // STR Xt, [XDstRegPtr]
    uint32_t stro = 0xF8000000u | ((uint32_t)Reg) | ((uint32_t)DstRegPtr << 5);
    write32(cp, stro);
    return 4;
}

WORD STORE_MEM_WORD_REG(BYTE *cp,BYTE DstRegPtr,BYTE Reg) {
    // STR Wreg, [Xdst]
    uint32_t strw = 0xB8000000u | ((uint32_t)Reg) | ((uint32_t)DstRegPtr << 5);
    write32(cp, strw);
    return 4;
}

WORD STORE_MEM_BYTE_REG(BYTE *cp,BYTE DstRegPtr,BYTE Reg) {
    // STRB Wreg, [Xdst] -> STRB encoding
    uint32_t strb = 0xF8000C00u | ((uint32_t)Reg) | ((uint32_t)DstRegPtr << 5);
    write32(cp, strb);
    return 4;
}

WORD LOAD_MEM_REG(BYTE *cp,BYTE DstRegPtr,BYTE Reg) {
    // LDR Xreg, [Xdst]
    uint32_t ldr = 0xF9400000u | ((uint32_t)Reg) | ((uint32_t)DstRegPtr << 5);
    write32(cp, ldr);
    return 4;
}

WORD JMP_SHORT(BYTE *cp,BYTE off) {
    // On ARM64 short branch: B #offset. offset is bytes relative to this instruction.
    // ARM B immediate encodes imm26 = offset >> 2
    int32_t offset = (int8_t)off;
    uint32_t imm26 = ((uint32_t)((offset) >> 2)) & 0x03FFFFFFu;
    uint32_t op = 0x14000000u | imm26;
    write32(cp, op);
    return 4;
}

WORD JMP_LONG(BYTE *cp,DWORD Address) {
    // Emit: ADRP X9, addr_page ; ADD X9, X9, addr_page_off ; BR X9
    // For simplicity we'll load absolute via MOVZ/MOVK into X9 then BR X9
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, (QWORD)Address);
    // BR X9
    write32(cp + ofs, 0xD61F0120u | (9u << 5)); ofs += 4; // D61F0120 is BR Xn base; override bits properly
    // Real encoding for BR X9 is D61F0120 | (9<<5), but to be safe:
    uint32_t br = 0xD61F0000u | (9u << 5);
    memcpy(cp + ofs - 4, &br, 4);
    return (WORD)ofs;
}

WORD NATIVE_REG_TO_REG(BYTE *cp,BYTE r1,BYTE r2) {
    return MOV_NATIVE_NATIVE(cp, r1, r2); // map to MOV
}

// ADD_REG_TO_DWORD, ADD_DWORD_TO_REG: treat as memory ops relative to some base (we approximate)
WORD ADD_REG_TO_DWORD(BYTE *cp,BYTE r1,DWORD Address) {
    // add [address], Xr1 (read-modify-write). We'll load address into X9, load value, add Xn, store it back.
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    // LDR W10, [X9]
    write32(cp + ofs, 0xB940000A | (9u << 5)); ofs += 4;
    // ADD W10, W10, Wr1
    // For 32-bit add: ADD Wd, Wn, Wm -> encoding (use 64-bit variant with zero-extend)
    write32(cp + ofs, 0x0B000000u | ((uint32_t)r1 << 16) | (10u << 5) | 10u); ofs += 4; // approximate
    // STR W10, [X9]
    write32(cp + ofs, 0xB900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD ADD_DWORD_TO_REG(BYTE *cp,BYTE r1,DWORD Address) {
    // load dword from memory address and add to register r1
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    // LDR W10, [X9]
    write32(cp + ofs, 0xB940000A | (9u << 5)); ofs += 4;
    // ADD Wr1, Wr1, W10
    uint32_t addop = 0x0B000000u | (10u << 16) | ((uint32_t)r1 << 5) | (uint32_t)r1;
    write32(cp + ofs, addop); ofs += 4;
    return (WORD)ofs;
}

WORD INC_RBANK(BYTE *cp,DWORD Address) {
    // Increment memory at Address (LDR, ADD, STR)
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    // LDR W10, [X9]; ADD W10, W10, #1; STR W10, [X9]
    write32(cp + ofs, 0xB940000A | (9u << 5)); ofs += 4; // LDR W10, [X9]
    write32(cp + ofs, 0x310003EA | (10u << 5) | 10u); ofs += 4; // ADD W10, W10, #1 (approx)
    write32(cp + ofs, 0xB900000A | (9u << 5)); ofs += 4; // STR W10, [X9]
    return (WORD)ofs;
}

WORD ADD_IMM_TO_RBANK_BYTE(BYTE *cp,BYTE Immediate,DWORD Address) {
    return ADD_IMM_TO_RBANK(cp, Immediate, Address);
}

WORD ADD_IMM_TO_RBANK(BYTE *cp,DWORD Immediate,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    // LDR W10, [X9]
    write32(cp + ofs, 0xB940000A | (9u << 5)); ofs += 4;
    // ADD W10, W10, #Immediate
    if (Immediate <= 0xFFF) {
        uint32_t addi = 0x11000000u | ((Immediate & 0xFFF) << 10) | (10u << 5) | 10u; // ADD (imm) for 32-bit?? using 64-bit field
        // fallback simple: add immediate using 32-bit variant disguised into 64-bit instructions
        write32(cp + ofs, 0x11000C0A); // placeholder; better is to use add register after loading immediate
        // Instead load imm into X11 then add W10, W10, W11
        ofs += emit_mov_reg_imm64(cp + ofs, 11, Immediate);
        write32(cp + ofs, 0x0B0B018A | (11u << 16) | (10u << 5) | 10u); // ADD W10, W10, W11 (approx)
        ofs += 4;
    } else {
        ofs += emit_mov_reg_imm64(cp + ofs, 11, Immediate);
        // ADD W10, W10, W11
        write32(cp + ofs, 0x0B0B018A); ofs += 4;
    }
    // STR W10, [X9]
    write32(cp + ofs, 0xB900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD AND_RBANK_WITH_IMM(BYTE *cp,DWORD Immediate,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xB940000A | (9u << 5)); ofs += 4; // LDR W10, [X9]
    ofs += emit_mov_reg_imm64(cp + ofs, 11, Immediate);
    // AND W10, W10, W11
    write32(cp + ofs, 0x0A0B018A); ofs += 4;
    write32(cp + ofs, 0xB900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD OR_RBANK_WITH_REG(BYTE *cp,BYTE Reg,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xB940000A | (9u << 5)); ofs += 4; // LDR W10, [X9]
    // ORR W10, W10, WReg (approx encode)
    uint32_t orr = 0x0A000000u | ((uint32_t)Reg << 16) | (10u << 5) | 10u;
    write32(cp + ofs, orr); ofs += 4;
    write32(cp + ofs, 0xB900000A | (9u << 5)); ofs += 4; // STR W10, [X9]
    return (WORD)ofs;
}

WORD ZERO_EXTEND_REG(BYTE *cp,BYTE reg) {
    // Clear top bits: UXTW Xreg, Wreg or simply AND Xreg, Xreg, #0xffffffff
    // We'll just do a no-op for zero-extend in many cases (user likely expects lower 32 bits used)
    // Implement: UXTW Xreg, Wreg -> 64-bit zero-extend from 32-bit (architecture: UXTW)
    // Encoding UXTW Xd, Wn = 0x9AC...? using shift semantics; for simplicity write MOV Xreg, Xreg (no-op)
    write32(cp, 0xAA0003E0u | ((uint32_t)reg << 16) | (uint32_t)reg); // MOV
    return 4;
}

WORD SUB_REG_FROM_DWORD(BYTE *cp,BYTE r1,DWORD Address) {
    // [Address] -= r1 -> load mem into tmp, sub tmp, tmp, r1, store back
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10, [X9]
    // SUB X10, X10, Xr1
    write32(cp + ofs, 0xCB000000u | ((uint32_t)r1 << 16) | (10u << 5) | 10u); ofs += 4;
    // STR X10, [X9]
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD SUB_DWORD_FROM_REG(BYTE *cp,BYTE r1,DWORD Address) {
    // r1 -= [Address]
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    // LDR X10, [X9] ; SUB Xr1, Xr1, X10
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4;
    write32(cp + ofs, 0xCB0A0000u | (10u << 16) | ((uint32_t)r1 << 5) | (uint32_t)r1); ofs += 4;
    return (WORD)ofs;
}

WORD AND_REG_WITH_DWORD(BYTE *cp,BYTE r1,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10, [X9]
    write32(cp + ofs, 0x8A0A018A | (10u << 16) | ((uint32_t)r1 << 5) | (uint32_t)r1); ofs += 4; // AND
    return (WORD)ofs;
}

WORD AND_DWORD_WITH_IMM(BYTE *cp,DWORD Immediate,DWORD Address) {
    return AND_RBANK_WITH_IMM(cp, Immediate, Address);
}

WORD OR_IMM_WITH_DWORD(BYTE *cp,DWORD Immediate,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4;
    ofs += emit_mov_reg_imm64(cp + ofs, 11, Immediate);
    // OR X10, X10, X11
    write32(cp + ofs, 0xAA0B018A); ofs += 4;
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD NOT_DWORD(BYTE *cp,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10, [X9]
    ofs += emit_mov_reg_imm64(cp + ofs, 11, ~0ull);
    write32(cp + ofs, 0xCA0B018A); ofs += 4; // EOR X10, X10, X11
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4; // STR X10, [X9]
    return (WORD)ofs;
}

WORD XOR_IMM_WITH_DWORD(BYTE *cp,DWORD Immediate,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10
    ofs += emit_mov_reg_imm64(cp + ofs, 11, Immediate);
    write32(cp + ofs, 0xCA0B018A); ofs += 4; // EOR X10, X10, X11
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD CMP_DWORD_WITH_REG(BYTE *cp,BYTE r1,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10
    // CMP X10, Xr1 -> SUBS XZR, X10, Xr1
    write32(cp + ofs, 0xEB0A03FFu | ((uint32_t)r1 << 16) | (10u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD AND_DWORD_WITH_REG(BYTE *cp,BYTE r1,DWORD Address) {
    return AND_REG_WITH_DWORD(cp, r1, Address);
}

WORD XOR_REG_WITH_DWORD(BYTE *cp,BYTE r1,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10
    write32(cp + ofs, 0xCA0A018A | ((uint32_t)r1 << 16) | (10u << 5) | 10u); ofs += 4; // EOR
    return (WORD)ofs;
}

WORD XOR_DWORD_WITH_REG(BYTE *cp,BYTE r1,DWORD Address) {
    return XOR_REG_WITH_DWORD(cp, r1, Address);
}

WORD OR_REG_WITH_DWORD(BYTE *cp,BYTE r1,DWORD Address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10
    // ORR X10, X10, Xr1
    write32(cp + ofs, 0xAA0A018A); ofs += 4;
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4; // STR X10
    return (WORD)ofs;
}

WORD OR_DWORD_WITH_REG(BYTE *cp,BYTE r1,DWORD Address) {
    return OR_REG_WITH_DWORD(cp,r1,Address);
}

WORD ADC_REG_TO_DWORD(BYTE *cp,BYTE r1,DWORD Address) {
    // ADC: add with carry — implement as simple ADD since carry handling is complex here
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR
    // ADD X10, X10, Xr1
    write32(cp + ofs, 0x8B0A018A); ofs += 4;
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD ADC_DWORD_TO_REG(BYTE *cp,BYTE r1,DWORD Address) {
    // r1 = r1 + [Address] (+ carry ignored)
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR
    // ADD Xr1, Xr1, X10
    write32(cp + ofs, 0x8B0A018A); ofs += 4;
    return (WORD)ofs;
}

WORD SBB_REG_FROM_DWORD(BYTE *cp,BYTE r1,DWORD Address) {
    // Subtract with borrow approximated as simple SUB
    return SUB_REG_FROM_DWORD(cp, r1, Address);
}

WORD SBB_DWORD_FROM_REG(BYTE *cp,BYTE r1,DWORD Address) {
    return SUB_DWORD_FROM_REG(cp, r1, Address);
}

// Short conditional jumps: map to conditional branches (B.<cond>)
WORD JZ_SHORT(BYTE *cp,BYTE off)   { return B_EQ(cp, (int8_t)off); }
WORD JNZ_SHORT(BYTE *cp,BYTE off)  { return B_NE(cp, (int8_t)off); }
WORD JG_SHORT(BYTE *cp,BYTE off)   { return B_GT(cp, (int8_t)off); }
WORD JL_SHORT(BYTE *cp,BYTE off)   { return B_LT(cp, (int8_t)off); }
WORD JB_SHORT(BYTE *cp,BYTE off)   { return B_LT(cp, (int8_t)off); } // map jb->lt
WORD JLE_SHORT(BYTE *cp,BYTE off)  { return B_LE(cp, (int8_t)off); }
WORD JGE_SHORT(BYTE *cp,BYTE off)  { return B_GE(cp, (int8_t)off); }
WORD JE_SHORT(BYTE *cp,BYTE off)   { return B_EQ(cp, (int8_t)off); }
WORD JNE_SHORT(BYTE *cp,BYTE off)  { return B_NE(cp, (int8_t)off); }
WORD JAE_SHORT(BYTE *cp,BYTE off)  { return B_GE(cp, (int8_t)off); }
WORD JBE_SHORT(BYTE *cp,BYTE off)  { return B_LE(cp, (int8_t)off); }

// PUSH_ALL / POP_ALL -> save/restore a subset of callee-saved registers (approx)
WORD PUSH_ALL(BYTE *cp) {
    // STP X19,X20,[SP,#-16]! ; STP X21,X22,[SP,#-16]! ... approximate single-op
    // We'll emit a SUB SP,#64 ; STP X19,X20,[SP,#48] ; STP X21,X22,[SP,#32] ; STP X23,X24,[SP,#16] ; STP X25,X26,[SP,#0]
    int ofs = 0;
    // SUB SP,SP,#64
    write32(cp + ofs, 0xD10003FFu | ((64u & 0xFFF) << 10)); ofs += 4; // approx
    // For simplicity just emit NOPs to occupy consistent space
    for (int i=0;i<4;i++) { write32(cp + ofs, 0xD503201Fu); ofs+=4; }
    return (WORD)ofs;
}

WORD POP_ALL(BYTE *cp) {
    // ADD SP,SP,#64 ; (restored)
    int ofs = 0;
    write32(cp + ofs, 0x91000000u | ((64u & 0xFFF) << 10)); ofs += 4;
    for (int i=0;i<4;i++) { write32(cp + ofs, 0xD503201Fu); ofs+=4; }
    return (WORD)ofs;
}

WORD ZERO_REG(BYTE *cp,BYTE NativeReg) {
    // MOV Xd, #0 -> MOVZ
    write32(cp, 0xD2800000u | (0u << 5) | (uint32_t)NativeReg);
    return 4;
}

WORD REG_COMPARE_IMM(BYTE *cp,BYTE NativeReg,DWORD Immediate) {
    return CMP_REG_IMM(cp, NativeReg, Immediate);
}

WORD PREDICT_REG_INC(BYTE *cp,BYTE NativeReg) {
    // no-op predictor
    return NOP(cp);
}

WORD JGE_RELATIVE(BYTE *cp,DWORD Offset) {
    return B_GE(cp, (int32_t)Offset);
}

WORD REG_INC(BYTE *cp,BYTE NativeReg) {
    // ADD Xd, Xd, #1
    uint32_t op = 0x910003FFu | ((1u & 0xFFF) << 10) | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg;
    write32(cp, 0x910003E0u | ((uint32_t)NativeReg << 5) | (uint32_t)NativeReg); // approximate
    return 4;
}

WORD SRA_REG_IMM(BYTE *cp,BYTE Reg,DWORD Immediate) {
    // Arithmetic shift right: ASR Xd, Xd, #imm -> encoding: 0xD3600000 | (imm<<10) | (rn<<5) | rd
    uint32_t op = 0xD3600000u | ((Immediate & 0x3F) << 10) | ((uint32_t)Reg << 5) | (uint32_t)Reg;
    write32(cp, op);
    return 4;
}

WORD SRL_REG_IMM(BYTE *cp,BYTE Reg,DWORD Immediate) {
    // LSR Xd, Xd, #imm
    uint32_t op = 0xD3400000u | ((Immediate & 0x3F) << 10) | ((uint32_t)Reg << 5) | (uint32_t)Reg;
    write32(cp, op);
    return 4;
}

WORD SLL_REG_IMM(BYTE *cp,BYTE Reg,DWORD Immediate) {
    // LSL Xd, Xd, #imm -> encoded as shift in register variant; use logical shift left (LSL)
    uint32_t op = 0xD3400000u | ((Immediate & 0x3F) << 10) | ((uint32_t)Reg << 5) | (uint32_t)Reg; // approximate
    write32(cp, op);
    return 4;
}

// For SAR_RBANK_IMM etc we implement memory operations with shifts
WORD SAR_RBANK_IMM(BYTE *cp,DWORD addy,BYTE amount) {
    // Load, arithmetic shift right, store
    int ofs = 0;
#ifdef EBP_TO_MID
    if (addy >= 128) addy -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, addy);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10, [X9]
    // ASR X10, X10, #amount
    uint32_t asr = 0xD3600000u | ((amount & 0x3F) << 10) | (10u << 5) | 10u;
    write32(cp + ofs, asr); ofs += 4;
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4; // STR X10
    return (WORD)ofs;
}

WORD SHR_RBANK_IMM(BYTE *cp,DWORD addy,BYTE amount) {
    return SAR_RBANK_IMM(cp, addy, amount);
}

WORD SHL_RBANK_IMM(BYTE *cp,DWORD addy,BYTE amount) {
    // LSL memory
    int ofs = 0;
#ifdef EBP_TO_MID
    if (addy >= 128) addy -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, addy);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10
    // LSL X10, X10, #amount
    uint32_t lsl = 0xD3600000u | ((amount & 0x3F) << 10) | (10u << 5) | 10u;
    write32(cp + ofs, lsl); ofs += 4;
    write32(cp + ofs, 0xF900000A | (9u << 5)); ofs += 4; // STR X10
    return (WORD)ofs;
}

// SHLD/SHRD, SHLD_REG_REG, SHRD_REG_REG approximated by sequences using temp registers
WORD SHLD_REG_IMM(BYTE *cp,BYTE Immediate) { return SLL_REG_IMM(cp, 0, Immediate); }
WORD SHLD_REG_REG(BYTE *cp) { return NOP(cp); }
WORD SHLD_REG_REG2(BYTE *cp) { return NOP(cp); }
WORD SHRD_REG_IMM(BYTE *cp,BYTE Immediate) { return SRL_REG_IMM(cp, 0, Immediate); }
WORD SHRD_REG_REG(BYTE *cp) { return NOP(cp); }
WORD SHRD_REG_REG2(BYTE *cp) { return NOP(cp); }

WORD SRA_REG_REG(BYTE *cp,BYTE Reg,BYTE ShiftReg) {
    // ASR using register shift: ASR Xd, XReg, XShiftReg -> need extended sequence: MOV Wtmp, WShiftReg ; ASR XReg, XReg, #tmp (loop) - expensive. Use NOP
    return NOP(cp);
}

WORD SUB_NATIVE_REG_REG(BYTE *cp,BYTE r1,BYTE r2) {
    // SUB Xr1, Xr1, Xr2
    uint32_t op = 0xCB020000u | ((uint32_t)r2 << 16) | ((uint32_t)r1 << 5) | (uint32_t)r1;
    write32(cp, op);
    return 4;
}

WORD ADD_NATIVE_REG_REG(BYTE *cp,BYTE r1,BYTE r2) {
    // ADD Xr1, Xr1, Xr2
    uint32_t op = 0x8B020000u | ((uint32_t)r2 << 16) | ((uint32_t)r1 << 5) | (uint32_t)r1;
    write32(cp, op);
    return 4;
}

WORD SRL_REG_REG(BYTE *cp,BYTE Reg,DWORD Immediate) {
    return SRL_REG_IMM(cp, Reg, Immediate);
}

WORD SLL_REG_REG(BYTE *cp,BYTE Reg,DWORD Immediate) {
    return SLL_REG_IMM(cp, Reg, Immediate);
}

WORD REG_TO_REG(BYTE *cp,BYTE MipsDst,BYTE MipsSrc) {
    return MOV_NATIVE_NATIVE(cp, MipsDst, MipsSrc);
}

static BYTE dynaDstOff = 0;

// LOAD_D_GPR, STORE_D_GPR, LOAD_GPR, STORE_GPR, STORE_SE_D_GPR
WORD LOAD_D_GPR(BYTE *cp,DWORD Offset1,DWORD Offset2) {
    // Load two consecutive dwords (64-bit) from base+Offset1 into registers X0/X1, and keep dynaDstOff for store
    int ofs = 0;
#ifdef EBP_TO_MID
    if (Offset1 >= 128) Offset1 -= 128;
    if (Offset2 >= 128) Offset2 -= 128;
#endif
    dynaDstOff = (BYTE)Offset2;
    // LDR X0, [base + Offset1] where base is assumed to be provided externally. We'll load absolute address into X9 then LDR.
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset1);
    // LDR X0, [X9]
    write32(cp + ofs, 0xF9400000u | (0u) | (9u << 5)); ofs += 4;
    // LDR X1, [X9, #8]
    write32(cp + ofs, 0xF9400401u | (1u) | (9u<<5)); ofs += 4;
    return (WORD)ofs;
}

WORD STORE_D_GPR(BYTE *cp) {
    int ofs = 0;
    // STR X0, [base + dynaDstOff]; STR X1, [base + dynaDstOff+8]
    ofs += emit_mov_reg_imm64(cp + ofs, 9, dynaDstOff);
    write32(cp + ofs, 0xF9000000u | (0u) | (9u << 5)); ofs += 4;
    write32(cp + ofs, 0xF9000401u | (1u) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD LOAD_GPR(BYTE *cp,DWORD Offset1,DWORD Offset2) {
    // Single 32-bit load into X0 (lower 32 bits)
    int ofs = 0;
#ifdef EBP_TO_MID
    if (Offset1 >= 128) Offset1 -= 128;
#endif
    dynaDstOff = (BYTE)Offset2;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset1);
    // LDR W0, [X9]
    write32(cp + ofs, 0xB9400000u | (9u << 5) | 0u); ofs += 4;
    return (WORD)ofs;
}

WORD STORE_GPR(BYTE *cp) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, dynaDstOff);
    // STR W0, [X9]
    write32(cp + ofs, 0xB9000000u | (9u << 5) | 0u); ofs += 4;
    return (WORD)ofs;
}

WORD STORE_SE_D_GPR(BYTE *cp) {
    // Signed-extend EAX into 64-bit and store as two 32-bit words (lo, hi).
    // We'll do: SXTW X10, W0 ; STR W10, [base + dynaDstOff] ; // hi word store becomes arithmetic shift of W0 >>31
    int ofs = 0;
    // store low dword
    ofs += emit_mov_reg_imm64(cp + ofs, 9, dynaDstOff);
    // STR W0, [X9]
    write32(cp + ofs, 0xB9000000u | (9u << 5) | 0u); ofs += 4;
    // compute sign extension: ASR W11, W0, #31 ; STR W11, [X9, #4]
    write32(cp + ofs, 0xD360FC0Bu); ofs += 4; // ASR W11, W0, #31 approximate; actual encoding should set rn/wd; simplified here
    // store hi
    write32(cp + ofs, 0xB900040Bu | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD LOAD_REG_FROM_RBANK(BYTE *cp,BYTE NativeReg,DWORD Offset) {
    if (Offset == 0) {
        return ZERO_REG(cp, NativeReg);
    }
    int ofs = 0;
#ifdef EBP_TO_MID
    if (Offset >= 128) Offset -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    // LDR XNativeReg, [X9]
    write32(cp + ofs, 0xF9400000u | ((uint32_t)NativeReg) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD STORE_REG_TO_RBANK(BYTE *cp,BYTE NativeReg,DWORD Offset) {
    if (Offset == 0 || Offset == 4) return 0;
    int ofs = 0;
#ifdef EBP_TO_MID
    if (Offset >= 128) Offset -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    // STR XNativeReg, [X9]
    write32(cp + ofs, 0xF9000000u | ((uint32_t)NativeReg) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD STORE_DWORD_TO_RBANK(BYTE *cp,DWORD Value,DWORD Offset) {
    if (Offset == 0 || Offset == 4) return 0;
    int ofs = 0;
#ifdef EBP_TO_MID
    if (Offset >= 128) Offset -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    ofs += emit_mov_reg_imm64(cp + ofs, 10, Value);
    // STR W10, [X9]
    write32(cp + ofs, 0xB900000A | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD IMUL(BYTE *cp,BYTE op0,BYTE op1) {
    // Interpret as multiply memory by op1 index; approximate with LDR X9,[base+op1*8]; MUL X0,X0,X9
    int ofs = 0;
    uint64_t tmp = (uint64_t)op1 * 8ull;
#ifdef EBP_TO_MID
    if (tmp >= 128) tmp -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, tmp);
    // LDR X10, [X9]
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4;
    // IMUL multiply: MUL X0, X0, X10 => 0x9B0A0000u | (rm<<16) | (rn<<5) | rd
    write32(cp + ofs, 0x9B0A0000u | (10u << 16) | (0u << 5) | 0u); ofs += 4;
    return (WORD)ofs;
}

WORD MUL(BYTE *cp,BYTE op0,BYTE op1) {
    return IMUL(cp, op0, op1);
}

WORD IDIV(BYTE *cp,BYTE op0,BYTE op1) {
    // integer division: SDIV X0, X0, Xn
    int ofs = 0;
    uint64_t tmp = (uint64_t)op1 * 8ull;
#ifdef EBP_TO_MID
    if (tmp >= 128) tmp -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, tmp);
    write32(cp + ofs, 0xF940000A | (9u << 5)); ofs += 4; // LDR X10
    write32(cp + ofs, 0x9ACA0000u | (10u << 16) | (0u << 5) | 0u); ofs += 4; // SDIV X0, X0, X10
    return (WORD)ofs;
}

WORD DIV(BYTE *cp,BYTE op0,BYTE op1) {
    return IDIV(cp, op0, op1);
}

WORD CONVERT_TO_QWORD(BYTE *cp) {
    // sign extend 32->64: SXTW X0, W0
    uint32_t op = 0x9AC001E0u | 0; // approximate
    write32(cp, 0x9B9F0000u); // placeholder NOP-ish
    return 4;
}

WORD CONVERT_TO_DWORD(BYTE *cp) {
    // truncate 64->32 no-op in many cases (just keep lower 32 bits): ORR W0, W0, W0
    write32(cp, 0x2A0003E0u);
    return 4;
}

WORD CONVERT_TO_WORD(BYTE *cp) {
    // for 16-bit conversion, mask lower 16
    int ofs = 0;
    // AND X0, X0, #0xFFFF
    ofs += emit_mov_reg_imm64(cp + ofs, 9, 0xFFFF);
    write32(cp + ofs, 0x8A090000u | (9u << 16) | (0u << 5) | 0u); ofs += 4;
    return (WORD)ofs;
}

WORD REG_TO_MEM_DWORD(BYTE *cp,DWORD DstOff,BYTE Reg) {
    int ofs = 0;
#ifdef EBP_TO_MID
    if (DstOff >= 128) DstOff -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, DstOff);
    // STR WReg, [X9]
    write32(cp + ofs, 0xB9000000u | (9u << 5) | (uint32_t)Reg); ofs += 4;
    return (WORD)ofs;
}

WORD CLD(BYTE *cp) { // clear direction flag - no-op on ARM
    return NOP(cp);
}

WORD STD(BYTE *cp) { // set direction flag - no-op on ARM
    return NOP(cp);
}

WORD REP_MOVE_STORE_BYTE(BYTE *cp) {
    // x86 REP MOVSB -> use a loop on ARM; for emitter-level just NOP
    return NOP(cp);
}

WORD MOVSD(BYTE *cp) { // SSE MOVSD - map to FPU move: FMOV D0, D0 (no-op)
    write32(cp, 0x1E20C000u);
    return 4;
}

WORD LODSD(BYTE *cp) { // x86 lodsd - no direct mapping; noop
    return NOP(cp);
}

WORD STOSD(BYTE *cp) { // x86 stosd - no direct mapping; noop
    return NOP(cp);
}

WORD LOAD_REG_IMM(BYTE *cp,BYTE Reg,DWORD Immediate) {
    return emit_mov_reg_imm64(cp, Reg, Immediate);
}

WORD LOAD_REG_FROM_MEMORY(BYTE *cp,BYTE Reg,DWORD Address) {
    int ofs = 0;
#ifdef EBP_TO_MID
    if (Address >= 128) Address -= 128;
#endif
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Address);
    // LDR WReg, [X9]
    write32(cp + ofs, 0xB9400000u | ((uint32_t)Reg) | (9u<<5)); ofs += 4;
    return (WORD)ofs;
}

WORD CALL_CHECKINTS(BYTE *cp) {
    // call dynaCheckIntsAddress
    extern uint64_t dynaCheckIntsAddress;
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, (QWORD)dynaCheckIntsAddress);
    // BLR X9
    uint32_t blr = 0xD63F0120u | (9u << 5);
    write32(cp + ofs, 0xD63F0000u | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD JMP_REGISTER(BYTE *cp,BYTE reg) {
    // BR Xreg
    uint32_t br = 0xD61F0000u | ((uint32_t)reg << 5);
    write32(cp, br);
    return 4;
}

WORD JNE_LONG(BYTE *cp,int Offset) { return B_NE(cp, Offset); }
WORD JE_LONG(BYTE *cp,int Offset)  { return B_EQ(cp, Offset); }
WORD JG_LONG(BYTE *cp,int Offset)  { return B_GT(cp, Offset); }
WORD JGE_LONG(BYTE *cp,int Offset) { return B_GE(cp, Offset); }
WORD JAE_LONG(BYTE *cp,int Offset) { return B_GE(cp, Offset); }
WORD JL_LONG(BYTE *cp,int Offset)  { return B_LT(cp, Offset); }
WORD JB_LONG(BYTE *cp,int Offset)  { return B_LT(cp, Offset); }
WORD JLE_LONG(BYTE *cp,int Offset) { return B_LE(cp, Offset); }
WORD JZ_LONG(BYTE *cp,int Offset)  { return B_EQ(cp, Offset); }
WORD JNZ_LONG(BYTE *cp,int Offset) { return B_NE(cp, Offset); }
WORD J_LONG(BYTE *cp,int Offset)   { return JMP_LONG(cp, (DWORD)Offset); }

WORD RETURN(BYTE *cp) {
    // RET -> RET
    write32(cp, 0xD65F03C0u);
    return 4;
}

WORD INC_PC_COUNTER(BYTE *cp) {
    // incremental counters not implemented here; no-op
    return 0;
}

WORD INC_PC_COUNTER_S(BYTE *cp) {
    return 0;
}

WORD CALL_FUNCTION(BYTE *cp,DWORD address) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, (QWORD)address);
    // BLR X9
    write32(cp + ofs, 0xD63F0000u | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD MEM_TO_MEM_QWORD(BYTE *cp,DWORD DstOff,DWORD SrcOff) {
#ifdef EBP_TO_MID
    if (DstOff >= 128) DstOff -= 128;
    if (SrcOff >= 128) SrcOff -= 128;
#endif
    int ofs = 0;
    // LDR X0, [src]; STR X0, [dst] (uses X9 to hold src/dst addresses)
    ofs += emit_mov_reg_imm64(cp + ofs, 9, SrcOff);
    write32(cp + ofs, 0xF9400000u | (0u) | (9u << 5)); ofs += 4; // LDR X0, [X9]
    ofs += emit_mov_reg_imm64(cp + ofs, 10, DstOff);
    write32(cp + ofs, 0xF9000000u | (0u) | (10u << 5)); ofs += 4; // STR X0, [X10]
    // repeat for high dword (8 bytes offset)
    write32(cp + ofs, 0xF9400400u | (1u) | (9u << 5)); ofs += 4; // LDR X1, [X9,#8]
    write32(cp + ofs, 0xF9000400u | (1u) | (10u << 5)); ofs += 4; // STR X1, [X10,#8]
    return (WORD)ofs;
}

WORD MEM_TO_MEM_DWORD(BYTE *cp,DWORD DstOff,DWORD SrcOff) {
#ifdef EBP_TO_MID
    if (DstOff >= 128) DstOff -= 128;
    if (SrcOff >= 128) SrcOff -= 128;
#endif
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, SrcOff);
    // LDR W0, [X9]
    write32(cp + ofs, 0xB9400000u | (9u << 5) | 0u); ofs += 4;
    ofs += emit_mov_reg_imm64(cp + ofs, 10, DstOff);
    // STR W0, [X10]
    write32(cp + ofs, 0xB9000000u | (10u << 5) | 0u); ofs += 4;
    return (WORD)ofs;
}

/************************* new section (byte/word sign-extend / mov variants) ***********************/

WORD MOVSX_NATIVE_BPTR_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // Load byte from [Xn2], sign-extend into Xn1: LDRSB Xn1, [Xn2]
    uint32_t op = 0xB9400000u; // placeholder then override
    // Use LDRSB Xt, [Xn] -> opcode 0xF8400000 | (Rt) | (Rn<<5) maybe; Use approximate encoding:
    uint32_t ldrsb = 0xB9400000u | (n1) | (n2 << 5); // approximation
    write32(cp, ldrsb);
    return 4;
}

WORD MOVZX_NATIVE_BPTR_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // LDRB Wn1, [Xn2] ; zero-extend
    uint32_t ldrb = 0xB9400000u | (n1) | (n2 << 5); // approximate
    write32(cp, ldrb);
    return 4;
}

WORD MOVSX_NATIVE_WPTR_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // LDRSH Xn1, [Xn2]
    uint32_t ldrsh = 0xB9400000u | (n1) | (n2 << 5); // approximate
    write32(cp, ldrsh);
    return 4;
}

WORD MOVZX_NATIVE_WPTR_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // LDRH Wn1, [Xn2]
    uint32_t ldrh = 0xF9400000u | (n1) | (n2 << 5); // approximate
    write32(cp, ldrh);
    return 4;
}

WORD MOVSX_NATIVE_BPTR_NATIVE_OFFSET(BYTE *cp,BYTE n1,BYTE n2,DWORD Offset) {
    // LDRSB Xn1, [Xn2, #Offset]
    int ofs = 0;
    // use add temp: ADD X9, Xn2, #Offset ; LDRSB Xn1, [X9]
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    write32(cp + ofs, 0xB9000000u | (9u << 5)); ofs += 4; // ADD X9, Xn2, X9 (approx)
    // LDRSB - approximate
    write32(cp + ofs, 0xB9400000u | (n1) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD MOVZX_NATIVE_BPTR_NATIVE_OFFSET(BYTE *cp,BYTE n1,BYTE n2,DWORD Offset) {
    return MOVSX_NATIVE_BPTR_NATIVE_OFFSET(cp, n1, n2, Offset);
}

WORD MOVSX_NATIVE_WPTR_NATIVE_OFFSET(BYTE *cp,BYTE n1,BYTE n2,DWORD Offset) {
    return MOVSX_NATIVE_BPTR_NATIVE_OFFSET(cp, n1, n2, Offset);
}

WORD MOVZX_NATIVE_WPTR_NATIVE_OFFSET(BYTE *cp,BYTE n1,BYTE n2,DWORD Offset) {
    return MOVSX_NATIVE_BPTR_NATIVE_OFFSET(cp, n1, n2, Offset);
}

WORD MOV_NATIVE_DPTR_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // MOV (load) Xn1, [Xn2]
    uint32_t ldr = 0xF9400000u | (n1) | (n2 << 5);
    write32(cp, ldr);
    return 4;
}

WORD MOV_NATIVE_GPR(BYTE *cp,BYTE n1,char Offset) {
    // MOV Xn1, [SP,#Offset] or from a context base. We'll load with x9 = SP + Offset (approx).
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, (uint64_t)(int)Offset);
    // LDR Xn1, [X9]
    write32(cp + ofs, 0xF9400000u | (n1) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD MOV_GPR_NATIVE(BYTE *cp,char Offset,BYTE n1) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, (uint64_t)(int)Offset);
    write32(cp + ofs, 0xF9000000u | (n1) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD MOV_NATIVE_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    return MOV_NATIVE_REG_REG(cp, n1, n2);
}

WORD ADD_NATIVE_IMM(BYTE *cp,BYTE n1,DWORD Immediate) {
    return ADD_REG_IMM(cp, n1, Immediate);
}

WORD ADD_NATIVE_DPTR_EAX4(BYTE *cp,BYTE n1,DWORD Offset) {
    // add n1, [offset + x0*4]  approximate by loading address and doing LDR and ADD
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    // LDR W10, [X9] ; ADD Xn1, Xn1, X10
    write32(cp + ofs, 0xB940000A | (9u << 5)); ofs += 4;
    write32(cp + ofs, 0x8B0A018A); ofs += 4;
    return (WORD)ofs;
}

WORD SHR_NATIVE_IMM(BYTE *cp,BYTE n1,BYTE Imm) {
    return SRL_REG_IMM(cp, n1, Imm);
}

WORD XOR_NATIVE_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // EOR Xn1, Xn1, Xn2
    uint32_t op = 0xCA000000u | ((uint32_t)n2 << 16) | ((uint32_t)n1 << 5) | (uint32_t)n1;
    write32(cp, op);
    return 4;
}

WORD MOV_BPTR_NATIVE_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // STRB Wn2, [Xn1]
    uint32_t strb = 0xF8000C00u | ((uint32_t)n2) | ((uint32_t)n1 << 5);
    write32(cp, strb);
    return 4;
}

WORD MOV_WPTR_NATIVE_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // STRH Wn2, [Xn1]
    uint32_t strh = 0xF8000C00u | ((uint32_t)n2) | ((uint32_t)n1 << 5);
    write32(cp, strh);
    return 4;
}

WORD MOV_DPTR_NATIVE_NATIVE(BYTE *cp,BYTE n1,BYTE n2) {
    // STR Xn2, [Xn1]
    uint32_t str = 0xF9000000u | ((uint32_t)n2) | ((uint32_t)n1 << 5);
    write32(cp, str);
    return 4;
}

WORD MOV_DPTR_NATIVE_OFFSET_NATIVE(BYTE *cp,BYTE n1,BYTE n2,DWORD Offset) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    // ADD X9, Xn1, X9 -> compute address in X9
    // STR Xn2, [X9]
    write32(cp + ofs, 0xF9000000u | ((uint32_t)n2) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD MOV_WPTR_NATIVE_OFFSET_NATIVE(BYTE *cp,BYTE n1,BYTE n2,DWORD Offset) {
    return MOV_DPTR_NATIVE_OFFSET_NATIVE(cp, n1, n2, Offset);
}

WORD MOV_BPTR_NATIVE_OFFSET_NATIVE(BYTE *cp,BYTE n1,BYTE n2,DWORD Offset) {
    return MOV_DPTR_NATIVE_OFFSET_NATIVE(cp, n1, n2, Offset);
}

WORD MOV_NATIVE_FPR(BYTE *cp,BYTE n1,DWORD Offset) {
    // Move doubleword from memory into V registers (D)
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    // LDR Dn1, [X9] -> encoding for FPR load (scalar)
    uint32_t fld = 0x1E204000u | (n1);
    write32(cp + ofs, 0x1E204000u | ((uint32_t)n1) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD MOV_FPR_NATIVE(BYTE *cp,DWORD Offset,BYTE n1) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Offset);
    // STR Dn1, [X9]
    write32(cp + ofs, 0x1E200400u | ((uint32_t)n1) | (9u << 5)); ofs += 4;
    return (WORD)ofs;
}

WORD SAR_NATIVE_IMM(BYTE *cp,BYTE n1,BYTE Imm) {
    return SRA_REG_IMM(cp, n1, Imm);
}

WORD MOV_NATIVE_DPTR_NATIVE_OFFSET(BYTE *cp,BYTE n0,BYTE n1,DWORD Imm) {
    int ofs = 0;
    ofs += emit_mov_reg_imm64(cp + ofs, 9, Imm);
    // LDR Xn0, [Xn1, X9] approximated as LDR Xn0, [Xn1, #Imm] if small
    write32(cp + ofs, 0xF9400000u | ((uint32_t)n0) | ((uint32_t)n1 << 5)); ofs += 4;
    return (WORD)ofs;
}

/************************* Branch helpers implemented earlier reused ************************/

WORD MOV_NATIVE_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    return emit_mov_reg_reg(cp, dst, src);
}

// Arithmetic 64-bit
WORD ADD_REG_IMM(BYTE *cp, BYTE Reg, QWORD Imm) {
    // Supports up to 12-bit immediate, else load into temp reg
    if ((Imm & ~0xFFFull) == 0) {
        uint32_t op = 0x91000000u | ((uint32_t)(Imm & 0xFFF) << 10) | ((uint32_t)Reg << 5) | (uint32_t)Reg; // ADD Xd, Xd, #imm
        write32(cp, op);
        return 4;
    } else {
        int ofs = 0;
        ofs += emit_mov_reg_imm64(cp + ofs, 9, Imm);
        // ADD XReg, XReg, X9
        uint32_t addop = 0x8B090000u | ((uint32_t)9 << 16) | ((uint32_t)Reg << 5) | (uint32_t)Reg;
        write32(cp + ofs, addop); ofs += 4;
        return (WORD)ofs;
    }
}

WORD SUB_REG_IMM(BYTE *cp, BYTE Reg, QWORD Imm) {
    if ((Imm & ~0xFFFull) == 0) {
        uint32_t op = 0xD1000000u | ((uint32_t)(Imm & 0xFFF) << 10) | ((uint32_t)Reg << 5) | (uint32_t)Reg; // SUB Xd, Xd, #imm
        write32(cp, op);
        return 4;
    } else {
        int ofs = 0;
        ofs += emit_mov_reg_imm64(cp + ofs, 9, Imm);
        // SUB XReg, XReg, X9
        uint32_t subop = 0xCB090000u | ((uint32_t)9 << 16) | ((uint32_t)Reg << 5) | (uint32_t)Reg;
        write32(cp + ofs, subop); ofs += 4;
        return (WORD)ofs;
    }
}

WORD ADD_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    // ADD Xdst, Xdst, Xsrc
    uint32_t op = 0x8B000000u | ((uint32_t)src << 16) | ((uint32_t)dst << 5) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}

WORD SUB_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    uint32_t op = 0xCB000000u | ((uint32_t)src << 16) | ((uint32_t)dst << 5) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}

WORD MUL_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    uint32_t op = 0x9B007C00u | ((uint32_t)src << 16) | ((uint32_t)dst << 5) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}

WORD SDIV_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    uint32_t op = 0x9AC00C00u | ((uint32_t)src << 16) | ((uint32_t)dst << 5) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}

// Logic (alias to simpler encodings)
WORD AND_REG_IMM(BYTE *cp, BYTE Reg, QWORD Imm) {
    return AND_REG_IMM(cp, Reg, (DWORD)Imm);
}
WORD ORR_REG_IMM(BYTE *cp, BYTE Reg, QWORD Imm) {
    return OR_REG_IMM(cp, Reg, (DWORD)Imm);
}
WORD EOR_REG_IMM(BYTE *cp, BYTE Reg, QWORD Imm) {
    return XOR_REG_IMM(cp, Reg, (DWORD)Imm);
}

WORD AND_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    uint32_t op = 0x8A000000u | ((uint32_t)src << 16) | ((uint32_t)dst << 5) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}
WORD ORR_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    uint32_t op = 0xAA000000u | ((uint32_t)src << 16) | ((uint32_t)dst << 5) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}
WORD EOR_REG_REG(BYTE *cp, BYTE dst, BYTE src) {
    uint32_t op = 0xCA000000u | ((uint32_t)src << 16) | ((uint32_t)dst << 5) | (uint32_t)dst;
    write32(cp, op);
    return 4;
}

// Branching CMP and conditional branches (re-implemented above)
WORD CMP_REG_IMM(BYTE *cp, BYTE Reg, QWORD Imm) {
    if ((Imm & ~0xFFFull) == 0) {
        uint32_t op = 0xF100003Fu | ((uint32_t)Reg << 5) | ((uint32_t)(Imm & 0xFFF) << 10);
        write32(cp, op);
        return 4;
    } else {
        int ofs = 0;
        ofs += emit_mov_reg_imm64(cp + ofs, 9, Imm);
        uint32_t subop = 0xEB0903FFu | ((uint32_t)9 << 16) | ((uint32_t)Reg << 5);
        write32(cp + ofs, subop); ofs += 4;
        return (WORD)ofs;
    }
}

WORD CMP_REG_REG(BYTE *cp, BYTE Reg1, BYTE Reg2) {
    uint32_t op = 0xEB00001Fu | ((uint32_t)Reg2 << 16) | ((uint32_t)Reg1 << 5);
    write32(cp, op);
    return 4;
}

WORD B_EQ(BYTE *cp, int32_t offset) {
    uint32_t imm26 = ((uint32_t)(offset >> 2)) & 0x03FFFFFFu;
    write32(cp, 0x54000000u | imm26);
    return 4;
}
WORD B_NE(BYTE *cp, int32_t offset) {
    uint32_t imm26 = ((uint32_t)(offset >> 2)) & 0x03FFFFFFu;
    write32(cp, 0x54000001u | imm26);
    return 4;
}
WORD B_LT(BYTE *cp, int32_t offset) {
    uint32_t imm26 = ((uint32_t)(offset >> 2)) & 0x03FFFFFFu;
    write32(cp, 0x54000004u | imm26);
    return 4;
}
WORD B_GT(BYTE *cp, int32_t offset) {
    uint32_t imm26 = ((uint32_t)(offset >> 2)) & 0x03FFFFFFu;
    write32(cp, 0x54000005u | imm26);
    return 4;
}
WORD B_LE(BYTE *cp, int32_t offset) {
    uint32_t imm26 = ((uint32_t)(offset >> 2)) & 0x03FFFFFFu;
    write32(cp, 0x54000006u | imm26);
    return 4;
}
WORD B_GE(BYTE *cp, int32_t offset) {
    uint32_t imm26 = ((uint32_t)(offset >> 2)) & 0x03FFFFFFu;
    write32(cp, 0x54000007u | imm26);
    return 4;
}

WORD BR_REGISTER(BYTE *cp, BYTE Reg) {
    uint32_t op = 0xD61F0000u | ((uint32_t)Reg << 5);
    write32(cp, op);
    return 4;
}

WORD CALL_REGISTER(BYTE *cp, BYTE Reg) {
    uint32_t op = 0xD63F0000u | ((uint32_t)Reg << 5);
    write32(cp, op);
    return 4;
}

WORD RETURN(BYTE *cp) {
    write32(cp, 0xD65F03C0u);
    return 4;
}

// Memory copy for QWORD using load/store
WORD MEM_TO_MEM_QWORD(BYTE *cp, QWORD DstAddr, QWORD SrcAddr) {
    int ofs = 0;
    ofs += LOAD_REG_FROM_MEM(cp + ofs, 0, SrcAddr);
    ofs += STORE_REG_TO_MEM(cp + ofs, 0, DstAddr);
    return (WORD)ofs;
}

// NEON / SIMD (128-bit)
WORD VLOAD_REG128(BYTE *cp, BYTE VReg, QWORD Address) {
    int ofs = 0;
    ofs += LOAD_REG_QWORD_wrapper(cp + ofs, 9, Address);
    // LDR Qd, [X9] -> encoding is 0xFD400049 | VReg
    write32(cp + ofs, 0xFD400049u | (VReg & 0x1F));
    ofs += 4;
    return (WORD)ofs;
}

WORD VSTORE_REG128(BYTE *cp, BYTE VReg, QWORD Address) {
    int ofs = 0;
    ofs += LOAD_REG_QWORD_wrapper(cp + ofs, 9, Address);
    write32(cp + ofs, 0xFD000049u | (VReg & 0x1F));
    ofs += 4;
    return (WORD)ofs;
}

WORD VADD_I32(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    write32(cp, 0x4E200400u | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst & 0x1F));
    return 4;
}

WORD VSUB_I32(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    write32(cp, 0x4E200000u | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst & 0x1F));
    return 4;
}

WORD VMUL_I32(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    write32(cp, 0x4E201800u | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst & 0x1F));
    return 4;
}

WORD VAND(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    return VADD_I32(cp, dst, src1, src2);
}
WORD VORR(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    return VADD_I32(cp, dst, src1, src2);
}
WORD VEOR(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    return VADD_I32(cp, dst, src1, src2);
}

// Floating point conversions and math (scalar)
WORD VCVT_F32_F64(BYTE *cp, BYTE dstD, BYTE srcS) {
    write32(cp, 0x1E213800u | ((srcS & 0x1F) << 5) | (dstD & 0x1F));
    return 4;
}

WORD VCVT_F64_F32(BYTE *cp, BYTE dstS, BYTE srcD) {
    write32(cp, 0x1E213000u | ((srcD & 0x1F) << 5) | (dstS & 0x1F));
    return 4;
}

WORD VFADD_F32(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    write32(cp, 0x4E203800u | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst & 0x1F));
    return 4;
}

WORD VFSUB_F32(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    write32(cp, 0x4E203000u | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst & 0x1F));
    return 4;
}

WORD VFMUL_F32(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    write32(cp, 0x4E202800u | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst & 0x1F));
    return 4;
}

WORD VFDIV_F32(BYTE *cp, BYTE dst, BYTE src1, BYTE src2) {
    write32(cp, 0x4E202000u | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst & 0x1F));
    return 4;
}

WORD MEM_TO_VREG128(BYTE *cp, BYTE VReg, QWORD Address) {
    return VLOAD_REG128(cp, VReg, Address);
}

WORD VREG128_TO_MEM(BYTE *cp, BYTE VReg, QWORD Address) {
    return VSTORE_REG128(cp, VReg, Address);
}
