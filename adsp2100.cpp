#include <cstdint>
#include <cstddef>
#include "adsp2100.h"

// ---------------------- Globals ----------------------
adsp2100_core adsp2100;
int adsp2100_icount = 0;

#if SUPPORT_2101_EXTENSIONS
static RX_CALLBACK rx_callback = nullptr;
static TX_CALLBACK tx_callback = nullptr;
#endif

// ---------------------- Memory Access ----------------------
inline uint8_t adsp2100_read_byte(uint32_t addr) { return dspReadByte(addr); }
inline uint16_t adsp2100_read_word(uint32_t addr) { return dspReadWord(addr); }
inline uint32_t adsp2100_read_dword(uint32_t addr) { return dspReadDWord(addr); }
inline void adsp2100_write_byte(uint32_t addr, uint8_t value) { dspWriteByte(addr, value); }
inline void adsp2100_write_word(uint32_t addr, uint16_t value) { dspWriteWord(addr, value); }
inline void adsp2100_write_dword(uint32_t addr, uint32_t value) { dspWriteDWord(addr, value); }

#define ADSP2100_RDMEM(A)        static_cast<uint32_t>(adsp2100_read_byte(A))
#define ADSP2100_RDMEM_WORD(A)   static_cast<uint32_t>(adsp2100_read_word(A))
#define ADSP2100_WRMEM(A, V)     adsp2100_write_byte(A, static_cast<uint8_t>(V))
#define ADSP2100_WRMEM_WORD(A,V) adsp2100_write_word(A, static_cast<uint16_t>(V))

// ---------------------- RX/TX Callbacks ----------------------
#if SUPPORT_2101_EXTENSIONS
inline void adsp2100_set_rx_callback(RX_CALLBACK cb) { rx_callback = cb; }
inline void adsp2100_set_tx_callback(TX_CALLBACK cb) { tx_callback = cb; }
inline int adsp2100_rx(int port) { return rx_callback ? rx_callback(port) : 0; }
inline void adsp2100_tx(int port, int32_t data) { if (tx_callback) tx_callback(port, data); }
#endif

// ---------------------- Flag Handling ----------------------
#define SSFLAG      0x80
#define MVFLAG      0x40
#define QFLAG       0x20
#define SFLAG       0x10
#define CFLAG       0x08
#define VFLAG       0x04
#define NFLAG       0x02
#define ZFLAG       0x01

inline void clr_flags() { adsp2100.astat &= adsp2100.astat_clear; }
inline void calc_z(int32_t r) { if ((r & 0xFFFF) == 0) adsp2100.astat |= ZFLAG; }
inline void calc_n(int32_t r) { adsp2100.astat |= ((r >> 14) & 0x02); }
inline void calc_v(int32_t s, int32_t d, int32_t r) { adsp2100.astat |= ((s ^ d ^ r ^ (r >> 1)) >> 13) & 0x04; }
inline void calc_c(int32_t r) { adsp2100.astat |= (r >> 13) & 0x08; }
inline void calc_c_sub(int32_t r) { adsp2100.astat |= (~r >> 13) & 0x08; }
inline void calc_nz(int32_t r) { clr_flags(); calc_n(r); calc_z(r); }
inline void calc_nzv(int32_t s,int32_t d,int32_t r){ clr_flags(); calc_n(r); calc_z(r); calc_v(s,d,r); }
inline void calc_nzvc(int32_t s,int32_t d,int32_t r){ clr_flags(); calc_n(r); calc_z(r); calc_v(s,d,r); calc_c(r); }
inline void calc_nzvc_sub(int32_t s,int32_t d,int32_t r){ clr_flags(); calc_n(r); calc_z(r); calc_v(s,d,r); calc_c_sub(r); }

// ---------------------- MSTAT ----------------------
#define MSTAT_BANK      0x01
#define MSTAT_REVERSE   0x02
#define MSTAT_STICKYV   0x04
#define MSTAT_SATURATE  0x08
#if SUPPORT_2101_EXTENSIONS
#define MSTAT_INTEGER   0x10
#define MSTAT_TIMER     0x20
#define MSTAT_GOMODE    0x40
#endif

inline void mstat_changed() {
    adsp2100_core* core = &adsp2100.r[adsp2100.mstat & MSTAT_BANK];
    adsp2100.astat_clear = (adsp2100.mstat & MSTAT_STICKYV) ? ~(CFLAG | NFLAG | ZFLAG) : ~(CFLAG | VFLAG | NFLAG | ZFLAG);
}

// ---------------------- PC Stack ----------------------
inline uint16_t pc_stack_top() { return adsp2100.pc_sp ? adsp2100.pc_stack[adsp2100.pc_sp-1] : adsp2100.pc_stack[0]; }
inline void set_pc_stack_top(uint16_t val) { if(adsp2100.pc_sp) adsp2100.pc_stack[adsp2100.pc_sp-1]=val; else adsp2100.pc_stack[0]=val; }
inline void pc_stack_push() { if(adsp2100.pc_sp<PC_STACK_DEPTH) { adsp2100.pc_stack[adsp2100.pc_sp++]=adsp2100.pc; adsp2100.sstat&=~PC_EMPTY; } else adsp2100.sstat|=PC_OVER; }
inline void pc_stack_push_val(uint16_t val) { if(adsp2100.pc_sp<PC_STACK_DEPTH) { adsp2100.pc_stack[adsp2100.pc_sp++]=val; adsp2100.sstat&=~PC_EMPTY; } else adsp2100.sstat|=PC_OVER; }
inline void pc_stack_pop() { if(adsp2100.pc_sp>0){adsp2100.pc_sp--; if(adsp2100.pc_sp==0)adsp2100.sstat|=PC_EMPTY;} adsp2100.pc=adsp2100.pc_stack[adsp2100.pc_sp]; }
inline uint16_t pc_stack_pop_val(){ if(adsp2100.pc_sp>0){adsp2100.pc_sp--; if(adsp2100.pc_sp==0)adsp2100.sstat|=PC_EMPTY;} return adsp2100.pc_stack[adsp2100.pc_sp]; }

// ---------------------- ALU Helpers ----------------------
inline int32_t alu_get_x(unsigned n) { return adsp2100.core->x[n].s; }
inline int32_t alu_get_y(unsigned n) { return adsp2100.core->y[n].s; }
inline void alu_set_x(unsigned n,int32_t val){adsp2100.core->x[n].s=val;}
inline void alu_set_y(unsigned n,int32_t val){adsp2100.core->y[n].s=val;}
inline int32_t alu_get_af(){return adsp2100.core->af.s;}
inline void alu_set_af(int32_t val){adsp2100.core->af.s=val;}

// ---------------------- ALU Operation ----------------------
void alu_op_af(int op){
    adsp2100_core* core = adsp2100.core;
    int32_t res=0;
    int32_t xop=(op>>8)&7;
    int32_t yop=(op>>11)&3;

    switch((op>>13)&15){
        case 0x00: res=alu_get_y(yop); calc_nz(res); break;
        case 0x01: { int32_t y=alu_get_y(yop); res=y+1; calc_nz(res); if(y==0x7fff) adsp2100.astat|=VFLAG; else if(y==0xffff) adsp2100.astat|=CFLAG; break;}
        case 0x02: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop)+(adsp2100.astat&CFLAG?1:0); res=x+y; calc_nzvc(x,y,res); break;}
        case 0x03: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop); res=x+y; calc_nzvc(x,y,res); break;}
        case 0x04: res=alu_get_y(yop)^0xffff; calc_nz(res); break;
        case 0x05: { int32_t y=alu_get_y(yop); res=-y; calc_nz(res); if(y==0x8000) adsp2100.astat|=VFLAG; if(y==0x0000) adsp2100.astat|=CFLAG; break;}
        case 0x06: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop); y-=(adsp2100.astat&CFLAG?1:0)-1; res=x-y; calc_nzvc_sub(x,y,res); break;}
        case 0x07: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop); res=x-y; calc_nzvc_sub(x,y,res); break;}
        case 0x08: { int32_t y=alu_get_y(yop); res=y-1; calc_nz(res); if(y==0x8000) adsp2100.astat|=VFLAG; else if(y==0x0000) adsp2100.astat|=CFLAG; break;}
        case 0x09: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop); res=y-x; calc_nzvc_sub(y,x,res); break;}
        case 0x0a: { int32_t x=alu_get_x(xop)-(adsp2100.astat&CFLAG?1:0)-1; int32_t y=alu_get_y(yop); res=y-x; calc_nzvc_sub(y,x,res); break;}
        case 0x0b: res=alu_get_x(xop)^0xffff; calc_nz(res); break;
        case 0x0c: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop); res=x&y; calc_nz(res); break;}
        case 0x0d: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop); res=x|y; calc_nz(res); break;}
        case 0x0e: { int32_t x=alu_get_x(xop); int32_t y=alu_get_y(yop); res=x^y; calc_nz(res); break;}
        case 0x0f: { int32_t x=alu_get_x(xop); res=(x&0x8000)?-x:x; if(x==0) adsp2100.astat|=ZFLAG; if(x==0x8000) adsp2100.astat|=NFLAG|VFLAG; adsp2100.astat&=~SFLAG; if(x&0x8000) adsp2100.astat|=SFLAG; break;}
    }

    if((adsp2100.mstat & MSTAT_SATURATE) && (adsp2100.astat&VFLAG))
        res=(adsp2100.astat&CFLAG)?-32768:32767;

    alu_set_af(res);
}

// ---------------------- MAC Operation ----------------------
void mac_op(int op){
    adsp2100_core* core=adsp2100.core;
    int32_t xreg=(op>>8)&7;
    int32_t yreg=(op>>11)&3;
    int64_t result;
    int32_t xval=alu_get_x(xreg);
    int32_t yval=alu_get_y(yreg);

    switch((op>>13)&15){
        case 0x0: result=(int64_t)xval*(int64_t)yval; break;
        case 0x1: result=(int64_t)xval*- (int64_t)yval; break;
        case 0x2: result=-(int64_t)xval*(int64_t)yval; break;
        case 0x3: result=-(int64_t)xval*-(int64_t)yval; break;
        case 0x4: result=(int64_t)xval*(int64_t)yval + alu_get_af(); break;
        case 0x5: result=(int64_t)xval*- (int64_t)yval + alu_get_af(); break;
        case 0x6: result=-(int64_t)xval*(int64_t)yval + alu_get_af(); break;
        case 0x7: result=-(int64_t)xval*-(int64_t)yval + alu_get_af(); break;
        default: result=0; break;
    }

    if(adsp2100.mstat & MSTAT_SATURATE){
        if(result>0x7FFFFFFFLL){ result=0x7FFFFFFFLL; adsp2100.astat|=VFLAG;}
        else if(result<-0x80000000LL){ result=-0x80000000LL; adsp2100.astat|=VFLAG;}
    }

    alu_set_af((int32_t)result);
}

// ---------------------- SHIFT Operation ----------------------
void shift_op(int op){
    int32_t shift=(op>>8)&0x1F;
    int32_t value=alu_get_af();
    int32_t res=0;

    switch((op>>13)&15){
        case 0x0: res=value<<shift; break;
        case 0x1: res=(uint32_t)value>>shift; break;
        case 0x2: res=value>>shift; break;
        case 0x3: res=(value<<shift)|((uint32_t)value>>(16-shift)); break;
        case 0x4: res=((uint32_t)value>>shift)|(value<<(16-shift)); break;
        default: res=value; break;
    }

    alu_set_af(res);
    calc_nz(res);
}

// ---------------------- SPECIAL ALU Operation ----------------------
void special_alu_op(int op){
    int32_t value=alu_get_af();
    int32_t res=0;

    switch((op>>8)&0xF){
        case 0x0: return;
        case 0x1: res=-value; break;
        case 0x2: res=(value<0)?-value:value; break;
        case 0x3: res=~value; break;
        case 0x4: if(value>0x7FFF){res=0x7FFF; adsp2100.astat|=VFLAG;} else if(value<-0x8000){res=-0x8000; adsp2100.astat|=VFLAG;} else res=value; break;
        case 0x5: { uint32_t v=(value<0)?~value:value; res=0; while((v&0x8000)==0 && v!=0){res++; v<<=1;} break; }
        default: res=value; break;
    }

    alu_set_af(res);
    calc_nz(res);
}
