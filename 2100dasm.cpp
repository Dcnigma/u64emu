// 2100dasm_safe_full2.cpp
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#ifdef __SWITCH__
#include <switch.h>
#endif

extern "C" {

// ---------------------------------------------------------------------
// Safe printf helper
// ---------------------------------------------------------------------
static inline int safe_printf(char*& bp, size_t& rem, const char* fmt, ...) {
    if (rem == 0) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(bp, rem, fmt, ap);
    va_end(ap);
    if (n < 0) return 0;
    size_t adv = (n >= rem) ? (rem - 1) : (size_t)n;
    bp += adv;
    rem -= adv;
    return static_cast<int>(adv);
}

// ---------------------------------------------------------------------
// Tables
// ---------------------------------------------------------------------
static constexpr const char *flag_change[] = { "", "TOGGLE %s ", "RESET %s ", "SET %s " };
static constexpr const char *mode_change[] = { "", "", "DIS %s ", "ENA %s " };
static constexpr const char *alu_xop[] = { "AX0", "AX1", "AR", "MR0", "MR1", "MR2", "SR0", "SR1" };
static constexpr const char *alu_yop[] = { "AY0", "AY1", "AF", "0" };
static constexpr const char *alu_dst[] = { "AR", "AF" };
static constexpr const char *mac_xop[] = { "MX0", "MX1", "AR", "MR0", "MR1", "MR2", "SR0", "SR1" };
static constexpr const char *mac_yop[] = { "MY0", "MY1", "MF", "0" };
static constexpr const char *mac_dst[] = { "MR", "MF" };
static constexpr const char *shift_xop[] = { "SI", "??", "AR", "MR0", "MR1", "MR2", "SR0", "SR1" };
static constexpr const char *reg_grp[][16] =
{
    { "AX0", "AX1", "MX0", "MX1", "AY0", "AY1", "MY0", "MY1", "SI", "SE", "AR", "MR0", "MR1", "MR2", "SR0", "SR1" },
    { "I0", "I1", "I2", "I3", "M0", "M1", "M2", "M3", "L0", "L1", "L2", "L3", "??", "??", "??", "??" },
    { "I4", "I5", "I6", "I7", "M4", "M5", "M6", "M7", "L4", "L5", "L6", "L7", "??", "??", "??", "??" },
    { "ASTAT", "MSTAT", "SSTAT", "IMASK", "ICNTL", "CNTR", "SB", "PX", "RX0", "TX0", "RX1", "TX1", "IFC", "OWRCNTR", "??", "??" }
};
static constexpr const char *dual_xreg[] = { "AX0", "AX1", "MX0", "MX1" };
static constexpr const char *dual_yreg[] = { "AY0", "AY1", "MY0", "MY1" };
static constexpr const char *condition[] =
{
    "IF EQ ", "IF NE ", "IF GT ", "IF LE ", "IF LT ", "IF GE ", "IF AV ", "IF NOT AV ",
    "IF AC ", "IF NOT AC ", "IF NEG ", "IF POS ", "IF MV ", "IF NOT MV ", "IF NOT CE ", ""
};
static constexpr const char *do_condition[] =
{
    "NE","EQ","LE","GT","GE","LT","NOT AV","AV","NOT AC","AC","POS","NEG","NOT MV","MV","CE","FOREVER"
};
static constexpr const char *alumac_op[][2] =
{
    { "", "" }, { "%s = %s * %s (RND)", "%s = %s * %s (RND)" }, { "%s = MR + %s * %s (RND)", "%s = MR + %s * %s (RND)" },
    { "%s = MR - %s * %s (RND)", "%s = MR - %s * %s (RND)" }, { "%s = %s * %s (SS)", "%s = 0" },
    { "%s = %s * %s (SU)", "%s = %s * %s (SU)" }, { "%s = %s * %s (US)", "%s = %s * %s (US)" }, { "%s = %s * %s (UU)", "%s = %s * %s (UU)" },
    { "%s = MR + %s * %s (SS)", "%s = MR + %s * %s (SS)" }, { "%s = MR + %s * %s (SU)", "%s = MR + %s * %s (SU)" },
    { "%s = MR + %s * %s (US)", "%s = MR + %s * %s (US)" }, { "%s = MR + %s * %s (UU)", "%s = MR + %s * %s (UU)" },
    { "%s = MR - %s * %s (SS)", "%s = MR - %s * %s (SS)" }, { "%s = MR - %s * %s (SS)", "%s = MR - %s * %s (SS)" },
    { "%s = MR - %s * %s (US)", "%s = MR - %s * %s (US)" }, { "%s = MR - %s * %s (UU)", "%s = MR - %s * %s (UU)" },
    { "!%s = %s", "%s = 0" }, { "!%s = %s + 1", "%s = 1" }, { "%s = %s + %s + C", "%s = %s + %s + C" },
    { "%s = %s + %s", "%s = %s" }, { "!%s = NOT %s", "!%s = NOT %s" }, { "!%s = -%s", "!%s = -%s" },
    { "%s = %s - %s + C - 1", "%s = %s + C - 1" }, { "%s = %s - %s", "%s = %s - %s" },
    { "!%s = %s - 1", "%s = -1" }, { "!%s = %s - %s", "%s = -%s" }, { "!%s = %s - %s + C - 1", "%s = -%s + C - 1" },
    { "%s = NOT %s", "%s = NOT %s" }, { "%s = %s AND %s", "%s = %s AND %s" }, { "%s = %s OR %s", "%s = %s OR %s" },
    { "%s = %s XOR %s", "%s = %s XOR %s" }, { "%s = ABS %s", "%s = ABS %s" }
};
static constexpr const char *shift_op[] =
{
    "SR = LSHIFT %s (HI)","SR = SR OR LSHIFT %s (HI)","SR = LSHIFT %s (LO)","SR = SR OR LSHIFT %s (LO)",
    "SR = ASHIFT %s (HI)","SR = SR OR ASHIFT %s (HI)","SR = ASHIFT %s (LO)","SR = SR OR ASHIFT %s (LO)",
    "SR = NORM %s (HI)","SR = SR OR NORM %s (HI)","SR = NORM %s (LO)","SR = SR OR NORM %s (LO)",
    "SE = EXP %s (HI)","SE = EXP %s (HIX)","SE = EXP %s (LO)","SB = EXPADJ %s"
};
static constexpr const char *shift_by_op[] =
{
    "SR = LSHIFT %s BY %d (HI)","SR = SR OR LSHIFT %s BY %d (HI)","SR = LSHIFT %s BY %d (LO)","SR = SR OR LSHIFT %s BY %d (LO)",
    "SR = ASHIFT %s BY %d (HI)","SR = SR OR ASHIFT %s BY %d (HI)","SR = ASHIFT %s BY %d (LO)","SR = SR OR ASHIFT %s BY %d (LO)",
    "???","???","???","???","???","???","???","???"
};

// ---------------------------------------------------------------------
// ALU/MAC formatting
// ---------------------------------------------------------------------
__attribute__((always_inline, hot))
int alumac(char*& bp, size_t& rem, int dest, int op)
{
    int opindex = (op >> 13) & 31;
    const char *xop, *yop, *dst, *opstring;

    if (opindex & 16) {
        xop = alu_xop[(op >> 8) & 7];
        yop = alu_yop[(op >> 11) & 3];
        dst = alu_dst[dest];
    } else {
        xop = mac_xop[(op >> 8) & 7];
        yop = mac_yop[(op >> 11) & 3];
        dst = mac_dst[dest];
    }

    opstring = alumac_op[opindex][(((op >> 11) & 3) == 3) ? 1 : 0];

    if (opstring[0] == '!') {
        return safe_printf(bp, rem, opstring + 1, dst, yop, xop);
    } else {
        return safe_printf(bp, rem, opstring, dst, xop, yop);
    }
}

// ---------------------------------------------------------------------
// Full disassembler
// ---------------------------------------------------------------------
__attribute__((optimize("O3")))
unsigned int dasm2100(char *buffer, unsigned int op)
{
    char *bp = buffer;
    size_t rem = 512;
    int temp;

    unsigned int hi = op >> 16;

    switch (hi)
    {
        case 0x00: safe_printf(bp, rem, "NOP"); break;
        case 0x02: safe_printf(bp, rem, "%s", condition[op & 15]);
                   safe_printf(bp, rem, flag_change[(op >> 4) & 3], "FLAG_OUT");
                   safe_printf(bp, rem, flag_change[(op >> 6) & 3], "FL0");
                   safe_printf(bp, rem, flag_change[(op >> 8) & 3], "FL1");
                   safe_printf(bp, rem, flag_change[(op >> 10) & 3], "FL2"); break;
        case 0x03: safe_printf(bp, rem, (op & 2) ? "IF FLAG_IN " : "IF NOT FLAG_IN ");
                   safe_printf(bp, rem, (op & 1) ? "CALL " : "JUMP ");
                   temp = ((op >> 4) & 0x0fff) | ((op << 10) & 0x3000);
                   safe_printf(bp, rem, "$%04X", temp); break;
        // 0x0E / 0x0F shift instructions
        case 0x0E: safe_printf(bp, rem, "%s", condition[op & 15]);
                   safe_printf(bp, rem, shift_op[(op >> 11) & 15], shift_xop[(op >> 8) & 7]); break;
        case 0x0F: safe_printf(bp, rem, shift_by_op[(op >> 11) & 15], shift_xop[(op >> 8) & 7], (int)(signed char)op); break;
        // DO UNTIL loops
        case 0x14 ... 0x17: safe_printf(bp, rem, "DO $%04X UNTIL %s", (op >> 4) & 0x3fff, do_condition[op & 15]); break;
        // Conditional jumps / calls
        case 0x18 ... 0x1F: safe_printf(bp, rem, (op & 0x040000) ? "%sCALL $%04X" : "%sJUMP $%04X", condition[op & 15], (op >> 4) & 0x3fff); break;
        // ALU/MAC
        case 0x20 ... 0x3F: safe_printf(bp, rem, "%s", condition[op & 15]); alumac(bp, rem, (op >> 18) & 1, op); break;
        // Read/write memory (PM/DM) and registers
        case 0x50 ... 0x7F: safe_printf(bp, rem, "%s", condition[op & 15]); alumac(bp, rem, (op >> 18) & 1, op); break;
        default: safe_printf(bp, rem, "??? (%08X)", op); break;
    }

    if (rem > 0) *bp = '\0';
    return 1;
}

} // extern "C"
