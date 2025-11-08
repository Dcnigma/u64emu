#include "adsp2100.h"

ADSP2100 adsp2100;
int adsp2100_icount = 0;

#if SUPPORT_2101_EXTENSIONS
static RX_CALLBACK rx_callback = nullptr;
static TX_CALLBACK tx_callback = nullptr;

void adsp2100_set_rx_callback(RX_CALLBACK cb) { rx_callback = cb; }
void adsp2100_set_tx_callback(TX_CALLBACK cb) { tx_callback = cb; }

inline int adsp2100_rx(int port) { return rx_callback ? rx_callback(port) : 0; }
inline void adsp2100_tx(int port, int32_t data) { if (tx_callback) tx_callback(port, data); }
#endif

// ---------------------- FLAG HELPERS ----------------------
#define ZFLAG 0x01
#define NFLAG 0x02
#define VFLAG 0x04
#define CFLAG 0x08
#define SFLAG 0x10

inline void clr_flags() { adsp2100.astat &= adsp2100.astat_clear; }
inline void calc_z(int32_t r) { if ((r & 0xFFFF) == 0) adsp2100.astat |= ZFLAG; }
inline void calc_n(int32_t r) { adsp2100.astat |= ((r >> 14) & 0x02); }
inline void calc_v(int32_t s,int32_t d,int32_t r){ adsp2100.astat |= ((s^d^r^(r>>1)) >> 13)&VFLAG; }
inline void calc_c(int32_t r){ adsp2100.astat |= (r>>13)&CFLAG; }
inline void calc_c_sub(int32_t r){ adsp2100.astat |= (~r>>13)&CFLAG; }
inline void calc_nz(int32_t r){ clr_flags(); calc_n(r); calc_z(r); }
inline void calc_nzv(int32_t s,int32_t d,int32_t r){ clr_flags(); calc_n(r); calc_z(r); calc_v(s,d,r); }
inline void calc_nzvc(int32_t s,int32_t d,int32_t r){ clr_flags(); calc_n(r); calc_z(r); calc_v(s,d,r); calc_c(r); }
inline void calc_nzvc_sub(int32_t s,int32_t d,int32_t r){ clr_flags(); calc_n(r); calc_z(r); calc_v(s,d,r); calc_c_sub(r); }

// ---------------------- PC STACK ----------------------
inline void pc_stack_push() { if(adsp2100.pc_sp<PC_STACK_DEPTH){adsp2100.pc_stack[adsp2100.pc_sp++]=adsp2100.pc;} }
inline void pc_stack_pop() { if(adsp2100.pc_sp>0) adsp2100.pc=adsp2100.pc_stack[--adsp2100.pc_sp]; }
inline uint16_t pc_stack_top() { return adsp2100.pc_sp ? adsp2100.pc_stack[adsp2100.pc_sp-1] : adsp2100.pc_stack[0]; }

// ---------------------- ALU ----------------------
void alu_op_af(int op){
    int32_t res = adsp2100.core.ax0.s + adsp2100.core.ay0.s; // dummy, real decoding goes here
    alu_set_af(res);
    calc_nz(res);
}

// ---------------------- MAC / SHIFT ----------------------
void mac_op(int op){
    int32_t x = adsp2100.core.ax0.s;
    int32_t y = adsp2100.core.mx0.s;
    int64_t res = (int64_t)x * (int64_t)y;
    alu_set_af((int32_t)res);
}

void shift_op(int op){
    int32_t val = alu_get_af();
    int32_t shift = 1; // dummy
    alu_set_af(val << shift);
    calc_nz(val << shift);
}

// ---------------------- SPECIAL ----------------------
void special_alu_op(int op){
    int32_t val = alu_get_af();
    int32_t res = -val;
    alu_set_af(res);
    calc_nz(res);
}

// ---------------------- EXECUTE ----------------------
int adsp2100_execute(int cycles){
    adsp2100_icount = cycles;
    while(adsp2100_icount > 0){
        // fetch instruction (simplified)
        int op = 0; // fetch from memory (platform-specific)
        alu_op_af(op); // dummy execution
        adsp2100.pc++;
        adsp2100_icount--;
    }
    return cycles - adsp2100_icount;
}

// ---------------------- RESET ----------------------
void adsp2100_reset(void* param){
    adsp2100.pc = 0;
    adsp2100.pc_sp = 0;
    adsp2100.astat = 0;
    adsp2100.sstat = 0;
    adsp2100.mstat = 0;
    adsp2100_icount = 0;
}
