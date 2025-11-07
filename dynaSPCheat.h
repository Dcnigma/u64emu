#pragma once
#include <switch.h>
#include "dynaMemory.h"
#include "dynaNative.h"

extern BYTE* dynaRamPtr;
extern struct Registers* r;

#define PC_PTR x29
#define MEM_PTR x30

// Increment PC counter (dummy, implement as needed)
#define INC_PC_COUNTER(cp) 0

// ARM64 versions of macros
#define LOAD_REG(cp, reg, target) 0 // Placeholder, implement register mapping
#define LOAD_REG_IMM(cp, reg, imm) 0
#define ADD_REG_IMM(cp, reg, imm) 0
#define LDR_REG_MEM(cp, reg, addr) 0
#define STR_REG_MEM(cp, reg, addr) 0
#define LDR_REG64_MEM(cp, reg, addr) 0
#define STR_REG64_MEM(cp, reg, addr) 0

// ---------------- Smart Store Word to SP ----------------
WORD dynaOpSwSP(BYTE *cp, BYTE op0, DWORD Imm)
{
    WORD l = 0;

    l += INC_PC_COUNTER(cp + l);

    // Load SP base into PC_PTR (x29)
    l += LOAD_REG(cp + l, 29, PC_PTR);

    // Load GPR[op0] into MEM_PTR (x30)
    l += LOAD_REG_IMM(cp + l, MEM_PTR, (DWORD)&r->GPR[op0 * 2]);

    // Add offset for SP memory region
    l += ADD_REG_IMM(cp + l, PC_PTR, (DWORD)dynaRamPtr + Imm - 0x88000000);

    // Store 32-bit word
    l += STR_REG_MEM(cp + l, MEM_PTR, PC_PTR);

    return l;
}

// ---------------- Smart Store Double to SP ----------------
WORD dynaOpSdSP(BYTE *cp, BYTE op0, DWORD Imm)
{
    WORD l = 0;

    l += INC_PC_COUNTER(cp + l);

    // Load SP base into PC_PTR (x29)
    l += LOAD_REG(cp + l, 29, PC_PTR);

    // Load GPR[op0] pair into MEM_PTR (x30)
    l += LOAD_REG_IMM(cp + l, MEM_PTR, (DWORD)&r->GPR[op0 * 2]);

    // Add offset for SP memory region
    l += ADD_REG_IMM(cp + l, PC_PTR, (DWORD)dynaRamPtr + Imm - 0x88000000);

    // Store 64-bit doubleword
    l += STR_REG64_MEM(cp + l, MEM_PTR, PC_PTR);

    return l;
}

// ---------------- Smart Load Word from SP ----------------
WORD dynaOpLwSP(BYTE *cp, BYTE op0, DWORD Imm)
{
    WORD l = 0;

    l += INC_PC_COUNTER(cp + l);

    // Load SP base into MEM_PTR (x30)
    l += LOAD_REG(cp + l, 29, MEM_PTR);

    // Load GPR[op0] into PC_PTR (x29)
    l += LOAD_REG_IMM(cp + l, PC_PTR, (DWORD)&r->GPR[op0 * 2]);

    // Add offset for SP memory region
    l += ADD_REG_IMM(cp + l, MEM_PTR, Imm + (DWORD)dynaRamPtr - 0x88000000);

    // Load 32-bit word
    l += LDR_REG_MEM(cp + l, PC_PTR, MEM_PTR);

    return l;
}

// ---------------- Smart Load Double from SP ----------------
WORD dynaOpLdSP(BYTE *cp, BYTE op0, DWORD Imm)
{
    WORD l = 0;

    l += INC_PC_COUNTER(cp + l);

    // Load SP base into MEM_PTR (x30)
    l += LOAD_REG(cp + l, 29, MEM_PTR);

    // Load GPR[op0] into PC_PTR (x29)
    l += LOAD_REG_IMM(cp + l, PC_PTR, (DWORD)&r->GPR[op0 * 2]);

    // Add offset for SP memory region
    l += ADD_REG_IMM(cp + l, MEM_PTR, Imm + (DWORD)dynaRamPtr - 0x88000000);

    // Load 64-bit doubleword into GPR pair
    l += LDR_REG64_MEM(cp + l, PC_PTR, MEM_PTR);

    return l;
}
