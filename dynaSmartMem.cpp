#include <math.h>
#include "ki.h"
#include "DynaCompiler.h"
#include "dynaNative.h"
#include "dynaMemory.h"
#include <switch.h>

extern DWORD smart;
extern DWORD smart2;

#define OFFSET 0
#define EBP_OFFSET 128
#define EXTEND_64

DWORD dynaReadMap[256];
DWORD dynaWriteMap[256];

// ---------------- Memory Map Builders ----------------
void dynaBuildReadMap()
{
    for(DWORD i = 0; i < 256; i++)
        dynaReadMap[i] = (DWORD)m->rdRam - (i << 24);

    dynaReadMap[0x80] = ((DWORD)m->rdRam + 0x800000) - 0x80000000;
    dynaReadMap[0x0]  = ((DWORD)m->rdRam + 0x90000);
    dynaReadMap[0xb0] = ((DWORD)m->aiReg) - 0xb0000000;
    dynaReadMap[0xa8] = ((DWORD)m->aiReg) - 0xa8000000;
}

void dynaBuildWriteMap()
{
    for(DWORD i = 0; i < 256; i++)
        dynaWriteMap[i] = (DWORD)m->rdRam - (i << 24);

    dynaWriteMap[0x80] = ((DWORD)m->rdRam + 0x800000) - 0x80000000;
    dynaWriteMap[0x0]  = ((DWORD)m->rdRam + 0x90000);
    dynaWriteMap[0xb0] = ((DWORD)m->atReg) - 0xb0000000;
    dynaWriteMap[0xa8] = ((DWORD)m->atReg) - 0xa8000000;
}

// ---------------- ARM64 Helper Macros ----------------
#define ARM64_MOV_REG(dst, src)     l += emit_mov_reg(cp + l, dst, src)
#define ARM64_ADD_IMM(dst, imm)     l += emit_add_imm(cp + l, dst, imm)
#define ARM64_LDRB(dst, base, offset) l += emit_ldrb(cp + l, dst, base, offset)
#define ARM64_LDRH(dst, base, offset) l += emit_ldrh(cp + l, dst, base, offset)
#define ARM64_LDR(dst, base, offset)  l += emit_ldr(cp + l, dst, base, offset)
#define ARM64_STRB(src, base, offset) l += emit_strb(cp + l, src, base, offset)
#define ARM64_STRH(src, base, offset) l += emit_strh(cp + l, src, base, offset)
#define ARM64_STR(src, base, offset)  l += emit_str(cp + l, src, base, offset)
#define ARM64_SXTW(dst, src)          l += emit_sxtw(cp + l, dst, src)
#define ARM64_UXTB(dst, src)          l += emit_uxtb(cp + l, dst, src)
#define ARM64_UXTH(dst, src)          l += emit_uxth(cp + l, dst, src)
#define ARM64_ASR_IMM(dst, src, imm)  l += emit_asr_imm(cp + l, dst, src, imm)
#define ARM64_LDR_FPR(dst, base, offset) l += emit_ldr_fpr(cp + l, dst, base, offset)
#define ARM64_STR_FPR(src, base, offset) l += emit_str_fpr(cp + l, src, base, offset)

// ---------------- Smart Load Byte ----------------
WORD dynaOpSmartLb(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDRB(0, 1, 0);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_ASR_IMM(0, 0, 31);
    ARM64_MOV_REG(op0 * 8 + 4, 0);
#endif
    return l;
}

WORD dynaOpSmartLbU(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDRB(0, 1, 0);
    ARM64_UXTB(0, 0);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_MOV_REG(op0 * 8 + 4, 3);
#endif
    return l;
}

// ---------------- Smart Load Half ----------------
WORD dynaOpSmartLh(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDRH(0, 1, 0);
    ARM64_SXTW(0, 0);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_ASR_IMM(0, 0, 31);
    ARM64_MOV_REG(op0 * 8 + 4, 0);
#endif
    return l;
}

WORD dynaOpSmartLhU(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDRH(0, 1, 0);
    ARM64_UXTH(0, 0);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_MOV_REG(op0 * 8 + 4, 3);
#endif
    return l;
}

// ---------------- Smart Load Word ----------------
WORD dynaOpSmartLw(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDR(0, 1, Imm);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_ASR_IMM(0, 0, 31);
    ARM64_MOV_REG(op0 * 8 + 4, 0);
#endif
    return l;
}

// ---------------- Smart Load Double ----------------
WORD dynaOpSmartLd(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDR(0, 1, Imm);
    ARM64_LDR(3, 1, Imm + 4);
    ARM64_MOV_REG(op0 * 8, 0);
    ARM64_MOV_REG(op0 * 8 + 4, 3);

    return l;
}

// ---------------- Smart Store Byte ----------------
WORD dynaOpSmartSb(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaWriteMap[Page]);
    ARM64_STRB(2, 1, 0);
    return l;
}

// ---------------- Smart Store Half ----------------
WORD dynaOpSmartSh(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaWriteMap[Page]);
    ARM64_STRH(2, 1, 0);
    return l;
}

// ---------------- Smart Store Word ----------------
WORD dynaOpSmartSw(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaWriteMap[Page]);
    ARM64_STR(2, 1, 0);
    return l;
}

// ---------------- Smart Load/Store FPR ----------------
WORD dynaOpSmartLwc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDR_FPR(0, 1, 0);
    ARM64_MOV_REG(op0 * 4, 0);
    return l;
}

WORD dynaOpSmartLdc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_ADD_IMM(1, Imm);
    ARM64_ASR_IMM(0, 1, 24);
    ARM64_ADD_IMM(1, (DWORD)dynaReadMap[Page]);
    ARM64_LDR_FPR(0, 1, 0);
    ARM64_MOV_REG(op0 * 4, 0);
    ARM64_ADD_IMM(1, 4);
    ARM64_LDR_FPR(0, 1, 0);
    ARM64_MOV_REG(op0 * 4 + 4, 0);
    return l;
}
// ---------------- Smart Load Byte 2 ----------------
WORD dynaOpSmartLb2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpLb(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_LDRB(0, 1, Offset);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_ASR_IMM(0, 0, 31);
    ARM64_MOV_REG(op0 * 8 + 4, 0);
#endif
    return l;
}

WORD dynaOpSmartLbU2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpLbU(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_LDRB(0, 1, Offset);
    ARM64_UXTB(0, 0);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_MOV_REG(op0 * 8 + 4, 3);
#endif
    return l;
}

// ---------------- Smart Load Half 2 ----------------
WORD dynaOpSmartLh2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpLh(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_LDRH(0, 1, Offset);
    ARM64_SXTW(0, 0);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_ASR_IMM(0, 0, 31);
    ARM64_MOV_REG(op0 * 8 + 4, 0);
#endif
    return l;
}

WORD dynaOpSmartLhU2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpLhU(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_LDRH(0, 1, Offset);
    ARM64_UXTH(0, 0);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_MOV_REG(op0 * 8 + 4, 3);
#endif
    return l;
}

// ---------------- Smart Load Word 2 ----------------
WORD dynaOpSmartLw2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpLw(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_LDR(0, 1, Offset);
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_ASR_IMM(0, 0, 31);
    ARM64_MOV_REG(op0 * 8 + 4, 0);
#endif
    return l;
}

WORD dynaOpSmartLwU2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpLwU(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_LDR(0, 1, Offset);
    ARM64_UXTH(0, 0); // zero extend lower half word
    ARM64_MOV_REG(op0 * 8, 0);

#ifdef EXTEND_64
    ARM64_MOV_REG(op0 * 8 + 4, 3);
#endif
    return l;
}

// ---------------- Smart Load Double 2 ----------------
WORD dynaOpSmartLd2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpLd(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    if(!op0) return 0;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_LDR(0, 1, Offset);
    ARM64_LDR(3, 1, Offset + 4);
    ARM64_MOV_REG(op0 * 8, 0);
    ARM64_MOV_REG(op0 * 8 + 4, 3);

    return l;
}

// ---------------- Smart Store Byte/Word/Double 2 ----------------
WORD dynaOpSmartSb2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpSb(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 8);
    ARM64_STRB(2, 1, Offset);
    return l;
}

WORD dynaOpSmartSh2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpSh(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 8);
    ARM64_STRH(2, 1, Offset);
    return l;
}

WORD dynaOpSmartSw2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpSw(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 8);
    ARM64_STR(2, 1, Offset);
    return l;
}

WORD dynaOpSmartSd2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    if(Page == 0xb0) return dynaOpSd(cp, op0, op1, Imm);

    WORD l = 0;
    DWORD Offset = dynaReadMap[Page] + Imm;
    if(Page == 0 && op1 == 29) Offset = (DWORD)m->rdRam + Imm;

    smart2++;
    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 8);
    ARM64_MOV_REG(3, op0 * 8 + 4);
    ARM64_STR(2, 1, Offset);
    ARM64_STR(3, 1, Offset + 4);
    return l;
}

// ---------------- Smart Store/Load FPR ----------------
WORD dynaOpSmartSwc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 4);
    ARM64_ADD_IMM(1, Imm);
    ARM64_STR_FPR(2, 1, 0);
    return l;
}

WORD dynaOpSmartSdc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page)
{
    WORD l = 0;
    smart++;

    ARM64_MOV_REG(1, op1 * 8);
    ARM64_MOV_REG(2, op0 * 4);
    ARM64_STR_FPR(2, 1, 0);
    ARM64_STR_FPR(2, 1, 4);
    return l;
}
