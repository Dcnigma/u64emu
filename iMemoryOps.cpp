#include <switch.h>
#include <GLES2/gl2.h>
#include <math.h>
#include "ki.h"
#include "EmuObject1.h"
#include "mmDisplay.h"
#include "iMain.h"
#include "iCPU.h"
#include "iMemory.h"
#include "iRom.h"

#define _SHIFTR(v, s, w) ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))
#define VERBOSE_TLB

//-------------------- Load/Store Double-Word --------------------
void iOpLdl()
{
    u32 offset = (u32)(r->GPR[MAKE_RS*2] + MAKE_I);
    u64 data = iMemReadQWord(offset & 0xfffffff8);

    switch (7 - (offset % 8))
    {
        case 0: *(u64 *)&r->GPR[MAKE_RT*2] = data; break;
        case 1: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0x00000000000000ffULL) | (data << 8); break;
        case 2: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0x000000000000ffffULL) | (data << 16); break;
        case 3: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0x0000000000ffffffULL) | (data << 24); break;
        case 4: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0x00000000ffffffffULL) | (data << 32); break;
        case 5: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0x0000ffffffffffULL) | (data << 40); break;
        case 6: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0x0000ffffffffffffULL) | (data << 48); break;
        case 7: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0x00ffffffffffffffULL) | (data << 56); break;
    }
}

void iOpLdr()
{
    u32 offset = (u32)(r->GPR[MAKE_RS*2] + MAKE_I);
    u64 data = iMemReadQWord(offset & 0xfffffff8);

    switch (7 - (offset % 8))
    {
        case 7: *(u64 *)&r->GPR[MAKE_RT*2] = data; break;
        case 6: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0xff00000000000000ULL) | (data >> 8); break;
        case 5: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0xffff000000000000ULL) | (data >> 16); break;
        case 4: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0xffffff0000000000ULL) | (data >> 24); break;
        case 3: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0xffffffff00000000ULL) | (data >> 32); break;
        case 2: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0xffffffffff000000ULL) | (data >> 40); break;
        case 1: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0xffffffffffff0000ULL) | (data >> 48); break;
        case 0: *(u64 *)&r->GPR[MAKE_RT*2] = (*(u64 *)&r->GPR[MAKE_RT*2] & 0xffffffffffffff00ULL) | (data >> 56); break;
    }
}

//-------------------- Byte/Word/Double Loads --------------------
void iOpLb()  { *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)(s16)(s8)iMemReadByte((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpLh()  { *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)(s16)iMemReadWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }

void iOpLwl()
{
    u64 offset = (u64)(r->GPR[MAKE_RS*2] + MAKE_I);
    u32 data = iMemReadDWord((u32)(offset & 0xfffffffc));

    switch (3 - (offset % 4))
    {
        case 0: *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)data; break;
        case 1: *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)((r->GPR[MAKE_RT*2] & 0x000000ff) | (data << 8)); break;
        case 2: *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)((r->GPR[MAKE_RT*2] & 0x0000ffff) | (data << 16)); break;
        case 3: *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)((r->GPR[MAKE_RT*2] & 0x00ffffff) | (data << 24)); break;
    }
}

void iOpLw()  { *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)iMemReadDWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpLbu(){ *(u64 *)&r->GPR[MAKE_RT*2] = (u64)(u32)(u16)(u8)iMemReadByte((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpLhu(){ *(u64 *)&r->GPR[MAKE_RT*2] = (u64)(u32)(u16)iMemReadWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }

void iOpLwr()
{
    u64 offset = (u64)(r->GPR[MAKE_RS*2] + MAKE_I);
    u32 data = iMemReadDWord((u32)(offset & 0xfffffffc));

    switch (3 - (offset % 4))
    {
        case 0: *(s64 *)&r->GPR[MAKE_RT*2] = (*(s64 *)&r->GPR[MAKE_RT*2] & 0xffffff00) | (data >> 24); break;
        case 1: *(s64 *)&r->GPR[MAKE_RT*2] = (*(s64 *)&r->GPR[MAKE_RT*2] & 0xffff0000) | (data >> 16); break;
        case 2: *(s64 *)&r->GPR[MAKE_RT*2] = (*(s64 *)&r->GPR[MAKE_RT*2] & 0xff000000) | (data >> 8); break;
        case 3: *(s64 *)&r->GPR[MAKE_RT*2] = (s64)(s32)data; break;
    }
}

void iOpLwu() { *(u64 *)&r->GPR[MAKE_RT*2] = (u64)(u32)iMemReadDWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }

//-------------------- Byte/Word/Double Stores --------------------
void iOpSb() { iMemWriteByte((u8)r->GPR[MAKE_RT*2], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpSh() { iMemWriteWord((u16)r->GPR[MAKE_RT*2], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }

void iOpSwl()
{
    u64 offset = (u64)r->GPR[MAKE_RS*2] + MAKE_I;
    u32 old_data = iMemReadDWord((u32)(offset & 0xfffffffc));
    u32 data = 0;

    switch (3 - (offset % 4))
    {
        case 0: data = (u32)r->GPR[MAKE_RT*2]; break;
        case 1: data = (old_data & 0xff000000) | ((u32)r->GPR[MAKE_RT*2] >> 8); break;
        case 2: data = (old_data & 0xffff0000) | ((u32)r->GPR[MAKE_RT*2] >> 16); break;
        case 3: data = (old_data & 0xffffff00) | ((u32)r->GPR[MAKE_RT*2] >> 24); break;
    }

    iMemWriteDWord(data, (u32)(offset & 0xfffffffc));
}

void iOpSw() { iMemWriteDWord((u32)r->GPR[MAKE_RT*2], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }

//-------------------- LL/SC --------------------
void iOpLl()  { *(s64*)&r->GPR[MAKE_RT*2] = (s64)(s32)iMemReadDWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); r->CPR0[2*LLADDR] = (s64)(s32)(r->GPR[MAKE_RS*2] + MAKE_I); r->Llbit = 1; }
void iOpLld() { *(u64*)&r->GPR[MAKE_RT*2] = iMemReadQWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); r->CPR0[2*LLADDR] = (s64)(s32)(r->GPR[MAKE_RS*2] + MAKE_I); r->Llbit = 1; }

void iOpSc()  { if(r->Llbit) iMemWriteDWord((u32)r->GPR[MAKE_RT*2], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); *(s64*)&r->GPR[MAKE_RT*2] = (s64)(s8)r->Llbit; }
void iOpScd() { if(r->Llbit) iMemWriteQWord(*(u64*)&r->GPR[MAKE_RT*2], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); *(s64*)&r->GPR[MAKE_RT*2] = (s64)(s8)r->Llbit; }

//-------------------- Floating-point Memory --------------------
void iOpLwc1()  { r->FPR[MAKE_FT] = (s32)iMemReadDWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpLwc2()  { *(u64*)&r->CPR2[2*MAKE_RT] = (u64)((s32)iMemReadDWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I))); }
void iOpLldc1() { /* optional */ }
void iOpLdc1()  { u64 value = iMemReadQWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); if(r->CPR0[2*STATUS] & 0x04000000) r->FPR[MAKE_FT] = (s32)value; else { r->FPR[MAKE_FT+0] = (s32)value; r->FPR[MAKE_FT+1] = (s32)(value >> 32); } }
void iOpLdc2()  { *(u64*)&r->CPR2[2*MAKE_RT] = iMemReadQWord((u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }

void iOpSwc1()  { iMemWriteDWord((u32)r->FPR[MAKE_FT], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpSwc2()  { iMemWriteDWord((u32)r->CPR2[2*MAKE_RT], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpSdc1()  { u64 value = *(u64*)&r->FPR[MAKE_FT]; value = (value << 32) | (value >> 32); iMemWriteQWord(value, (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpSdc2()  { iMemWriteQWord(r->CPR2[2*MAKE_RT*2], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }
void iOpSd()    { iMemWriteQWord(*(u64*)&r->GPR[MAKE_RT*2], (u32)(r->GPR[MAKE_RS*2] + MAKE_I)); }

//-------------------- TLB Operations --------------------
void iOpTlbr()
{
    u64 index = r->CPR0[INDEX*2] & 0x1f;
    r->CPR0[PAGEMASK*2] = r->Tlb[index].hh;
    r->CPR0[ENTRYHI*2]  = (r->Tlb[index].hl & ~r->Tlb[index].hh);
    r->CPR0[ENTRYLO1*2] = (r->Tlb[index].lh | r->Tlb[index].g);
    r->CPR0[ENTRYLO0*2] = (r->Tlb[index].ll | r->Tlb[index].g);
}

void iOpTlbwi()
{
    u64 index = r->CPR0[2*INDEX];
    r->Tlb[index].hh = (u32)r->CPR0[2*PAGEMASK];
    r->Tlb[index].hl = (u32)(r->CPR0[2*ENTRYHI] & ~r->CPR0[2*PAGEMASK]);
    r->Tlb[index].lh = (u32)(r->CPR0[2*ENTRYLO1] & ~1);
    r->Tlb[index].ll = (u32)(r->CPR0[2*ENTRYLO0] & ~1);
    r->Tlb[index].g  = (u8)(0x01 & r->CPR0[2*ENTRYLO1] & r->CPR0[2*ENTRYLO0]);
}

void iOpTlbwr() { /* write random indexed entry; implement as needed */ }
void iOpTlbp()  { /* probe TLB for matching entry; implement as needed */ }

#pragma optimize("",on)
