#include "adsp2100_core.h"
#include <arm_neon.h>
#include <cstdint>

ADSPCORE adsp2100;

// ---------------------- NEON MAC / SHIFT ----------------------
inline void MAC_OP(ADSPCORE* c, int16_t src, int16_t coef) {
    int64_t acc = c->mr.mr;
    int32_t msrc = static_cast<int32_t>(src);
    int32_t mcoef = static_cast<int32_t>(coef);

    int64x2_t mac = vdupq_n_s64(acc);
    int64x2_t mul = vmovl_s32(vdup_n_s32(msrc * mcoef));
    mac = vaddq_s64(mac, mul);
    c->mr.mr = vgetq_lane_s64(mac, 0);
}

inline void SHIFT_OP(ADSPCORE* c, int16_t val, int shift) {
    if (shift > 0) {
        c->sr.sr0.s = val << shift;
    } else {
        c->sr.sr0.s = val >> (-shift);
    }
}

// ---------------------- OPCODE TABLE ----------------------
void* opcode_table[0x10000];

#define DEF_OP(code, label) opcode_table[code] = &&label

void init_opcode_table() {
    static bool initialized = false;
    if (initialized) return;

    for (int i = 0; i < 0x10000; i++)
        opcode_table[i] = &&op_nop;

    // Arithmetic
    DEF_OP(0x0000, op_add_ax0_ax1);
    DEF_OP(0x0001, op_sub_ax0_ax1);
    DEF_OP(0x0002, op_mac_ax0_mx0);
    DEF_OP(0x0003, op_shift_ax0_1);
    DEF_OP(0x0004, op_mac_ax0_mx1);
    DEF_OP(0x0005, op_shift_ax0_n);

    // Logical
    DEF_OP(0x0100, op_and_ax0_ax1);
    DEF_OP(0x0101, op_or_ax0_ax1);
    DEF_OP(0x0102, op_xor_ax0_ax1);
    DEF_OP(0x0103, op_not_ax0);

    // Memory
    DEF_OP(0x0200, op_load_ax0_mx0);
    DEF_OP(0x0201, op_store_ax0_mx0);
    DEF_OP(0x0202, op_load_ax1_mx1);
    DEF_OP(0x0203, op_store_ax1_mx1);

    // Control flow
    DEF_OP(0x0300, op_jmp);
    DEF_OP(0x0301, op_call);
    DEF_OP(0x0302, op_rts);
    DEF_OP(0x0303, op_bra);
    DEF_OP(0x0304, op_bsr);
    DEF_OP(0x0305, op_bnz);

    initialized = true;
}

// ---------------------- OPCODE HANDLERS ----------------------
op_nop:
    adsp2100.pc++;
    NEXT_OP;

// --- ARITHMETIC ---
op_add_ax0_ax1:
    adsp2100.ax0.s += adsp2100.ax1.s;
    adsp2100.pc++;
    NEXT_OP;

op_sub_ax0_ax1:
    adsp2100.ax0.s -= adsp2100.ax1.s;
    adsp2100.pc++;
    NEXT_OP;

op_mac_ax0_mx0:
    MAC_OP(&adsp2100, adsp2100.ax0.s, adsp2100.mx0.s);
    adsp2100.pc++;
    NEXT_OP;

op_mac_ax0_mx1:
    MAC_OP(&adsp2100, adsp2100.ax0.s, adsp2100.mx1.s);
    adsp2100.pc++;
    NEXT_OP;

op_shift_ax0_1:
    SHIFT_OP(&adsp2100, adsp2100.ax0.s, 1);
    adsp2100.pc++;
    NEXT_OP;

op_shift_ax0_n:
    SHIFT_OP(&adsp2100, adsp2100.ax0.s, adsp2100.sr.sr0.s);
    adsp2100.pc++;
    NEXT_OP;

// --- LOGICAL ---
op_and_ax0_ax1:
    adsp2100.ax0.s &= adsp2100.ax1.s;
    adsp2100.pc++;
    NEXT_OP;

op_or_ax0_ax1:
    adsp2100.ax0.s |= adsp2100.ax1.s;
    adsp2100.pc++;
    NEXT_OP;

op_xor_ax0_ax1:
    adsp2100.ax0.s ^= adsp2100.ax1.s;
    adsp2100.pc++;
    NEXT_OP;

op_not_ax0:
    adsp2100.ax0.s = ~adsp2100.ax0.s;
    adsp2100.pc++;
    NEXT_OP;

// --- MEMORY ---
op_load_ax0_mx0:
    adsp2100.ax0.s = adsp2100.mx0.s;
    adsp2100.pc++;
    NEXT_OP;

op_store_ax0_mx0:
    adsp2100.mx0.s = adsp2100.ax0.s;
    adsp2100.pc++;
    NEXT_OP;

op_load_ax1_mx1:
    adsp2100.ax1.s = adsp2100.mx1.s;
    adsp2100.pc++;
    NEXT_OP;

op_store_ax1_mx1:
    adsp2100.mx1.s = adsp2100.ax1.s;
    adsp2100.pc++;
    NEXT_OP;

// --- CONTROL FLOW ---
op_jmp:
    adsp2100.pc = adsp2100.ir;
    NEXT_OP;

op_call:
    adsp2100.stack[adsp2100.sp++] = adsp2100.pc;
    adsp2100.pc = adsp2100.ir;
    NEXT_OP;

op_rts:
    adsp2100.pc = adsp2100.stack[--adsp2100.sp];
    NEXT_OP;

op_bra:
    adsp2100.pc += adsp2100.ir;
    NEXT_OP;

op_bsr:
    adsp2100.stack[adsp2100.sp++] = adsp2100.pc;
    adsp2100.pc += adsp2100.ir;
    NEXT_OP;

op_bnz:
    if (adsp2100.ax0.s != 0)
        adsp2100.pc += adsp2100.ir;
    else
        adsp2100.pc++;
    NEXT_OP;

// --- EXECUTION LOOP ---
int adsp2100_execute(int cycles) {
    int icount = cycles;
    init_opcode_table();

    NEXT_OP;

    while (icount > 0) {
        check_irqs(); // user-defined interrupt handling
        icount--;
    }

    return cycles - icount;
}

// --- HELPER MACROS ---
#define NEXT_OP goto *opcode_table[adsp2100.ir & 0xFFFF]
// ---------------------- CIRCULAR BUFFER MEMORY ----------------------
inline int16_t circ_load(ADSPCORE* c, int16_t* buffer, int index, int mask) {
    return buffer[index & mask];
}

inline void circ_store(ADSPCORE* c, int16_t* buffer, int index, int mask, int16_t val) {
    buffer[index & mask] = val;
}

// ---------------------- ADDITIONAL MAC VARIANTS ----------------------
inline void MAC_AX1_MX0(ADSPCORE* c) {
    MAC_OP(c, c->ax1.s, c->mx0.s);
}

inline void MAC_AX1_MX1(ADSPCORE* c) {
    MAC_OP(c, c->ax1.s, c->mx1.s);
}

inline void MAC_AX0_MX1(ADSPCORE* c) {
    MAC_OP(c, c->ax0.s, c->mx1.s);
}

// ---------------------- LOOP / CONDITIONAL ----------------------
op_bloop:
    if (--adsp2100.lc != 0) {
        adsp2100.pc = adsp2100.loop_addr;
    } else {
        adsp2100.pc++;
    }
    NEXT_OP;

op_blrz:
    if (adsp2100.ax0.s == 0) {
        adsp2100.pc = adsp2100.loop_addr;
    } else {
        adsp2100.pc++;
    }
    NEXT_OP;

op_bnr:
    if (adsp2100.ax0.s != 0) {
        adsp2100.pc += adsp2100.ir;
    } else {
        adsp2100.pc++;
    }
    NEXT_OP;

// ---------------------- I/O, TIMERS, SERIAL ----------------------
op_tcsr_read:
    adsp2100.ax0.s = adsp2100.timer_control;
    adsp2100.pc++;
    NEXT_OP;

op_tcsr_write:
    adsp2100.timer_control = adsp2100.ax0.s;
    adsp2100.pc++;
    NEXT_OP;

op_serial_read:
    adsp2100.ax0.s = adsp2100.serial_data;
    adsp2100.pc++;
    NEXT_OP;

op_serial_write:
    adsp2100.serial_data = adsp2100.ax0.s;
    adsp2100.pc++;
    NEXT_OP;

// ---------------------- OPCODE TABLE EXTENSION ----------------------
#define DEF_OP_EXT(code, label) opcode_table[0xFF00 + (code)] = &&label

void init_opcode_table_ext() {
    // Circular MAC variants
    DEF_OP_EXT(0x00, op_mac_ax1_mx0);
    DEF_OP_EXT(0x01, op_mac_ax1_mx1);
    DEF_OP_EXT(0x02, op_mac_ax0_mx1);

    // Looping
    DEF_OP_EXT(0x10, op_bloop);
    DEF_OP_EXT(0x11, op_blrz);
    DEF_OP_EXT(0x12, op_bnr);

    // Timers and I/O
    DEF_OP_EXT(0x20, op_tcsr_read);
    DEF_OP_EXT(0x21, op_tcsr_write);
    DEF_OP_EXT(0x22, op_serial_read);
    DEF_OP_EXT(0x23, op_serial_write);
}

// ---------------------- EXTENDED EXECUTION LOOP ----------------------
int adsp2100_execute_ext(int cycles) {
    int icount = cycles;
    init_opcode_table();
    init_opcode_table_ext();

    NEXT_OP;

    while (icount > 0) {
        check_irqs(); // user-defined interrupt handling
        icount--;
    }

    return cycles - icount;
}
