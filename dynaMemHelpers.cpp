#include <cstdint>
#include <cmath>        // for math.h
#include <cstring>      // for memcpy, if needed

#include "ki.h"
#include "iMain.h"
#include "iCPU.h"            // Core 4300 emulation routines
#include "dynaCompiler.h"
#include "iMemory.h"         // Memory emulation routines
#include "iRom.h"            // Rom (cart) emulation routines
#include "iATA.h"            // KI ATA-2 hard disk emulation

//---------------- Type definitions ----------------
using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;
using QWORD = uint64_t;
using sBYTE = int8_t;
using sWORD = int16_t;
using sDWORD = int32_t;
using sQWORD = int64_t;

//---------------- External variables ----------------
extern DWORD cheat;
extern struct CPUState* r;     // Make sure this points to your CPU state
extern DWORD iMemToDo;
extern void iATAUpdate();
extern void* iMemPhysReadAddr(DWORD address);
extern void* iMemPhysWriteAddr(DWORD address);
extern DWORD iMemReadDWord(DWORD address);
extern QWORD iMemReadQWord(DWORD address);
extern void iMemWriteDWord(DWORD value, DWORD address);
extern void iMemWriteQWord(QWORD value, DWORD address);

#define LLADDR 0

//--------------------------------------------------------
// Standard Load/Store Helpers
//--------------------------------------------------------
inline void helperLb(DWORD address, DWORD op0)
{
    if(__builtin_expect(op0==0,0))
        iMemPhysReadAddr(address);
    else
        *(sQWORD *)&r->GPR[op0*2] = (sQWORD)(sDWORD)(short)*(int8_t*)iMemPhysReadAddr(address);
}

inline void helperLbU(DWORD address, DWORD op0)
{
    if(__builtin_expect(op0==0,0))
        iMemPhysReadAddr(address);
    else
        *(sQWORD *)&r->GPR[op0*2] = (QWORD)(DWORD)*(uint8_t*)iMemPhysReadAddr(address);
}

inline void helperLh(DWORD address, DWORD op0)
{
    if(__builtin_expect(op0==0,0))
        iMemPhysReadAddr(address);
    else
        *(sQWORD *)&r->GPR[op0*2] = (sQWORD)(sDWORD)*(int16_t*)iMemPhysReadAddr(address);
}

inline void helperLhU(DWORD address, DWORD op0)
{
    if(__builtin_expect(op0==0,0))
        iMemPhysReadAddr(address);
    else
        *(QWORD *)&r->GPR[op0*2] = (QWORD)(DWORD)*(uint16_t*)iMemPhysReadAddr(address);
}

inline void helperLw(DWORD address, DWORD op0)
{
    if(__builtin_expect(op0==0,0))
        iMemPhysReadAddr(address);
    else
        *(sQWORD *)&r->GPR[op0*2] = (sQWORD)*(sDWORD*)iMemPhysReadAddr(address);
}

inline void helperLwU(DWORD address, DWORD op0)
{
    if(__builtin_expect(op0==0,0))
        iMemPhysReadAddr(address);
    else
        *(sQWORD *)&r->GPR[op0*2] = (QWORD)*(DWORD*)iMemPhysReadAddr(address);
}

inline void helperLd(DWORD address, DWORD op0)
{
    if(__builtin_expect(op0==0,0))
        iMemPhysReadAddr(address);
    else
        *(sQWORD *)&r->GPR[op0*2] = *(sQWORD*)iMemPhysReadAddr(address);
}

inline void helperSb(DWORD address, DWORD op0)
{
    *(uint8_t*)iMemPhysWriteAddr(address) = *(uint8_t*)&r->GPR[op0*2];
    if(iMemToDo & 0x8000) iATAUpdate();
}

inline void helperSh(DWORD address, DWORD op0)
{
    *(uint16_t*)iMemPhysWriteAddr(address) = *(uint16_t*)&r->GPR[op0*2];
    if(iMemToDo & 0x8000) iATAUpdate();
}

inline void helperSw(DWORD address, DWORD op0)
{
    *(DWORD*)iMemPhysWriteAddr(address) = *(DWORD*)&r->GPR[op0*2];
    if(iMemToDo & 0x8000) iATAUpdate();
}

inline void helperSd(DWORD address, DWORD op0)
{
    *(QWORD*)iMemPhysWriteAddr(address) = *(QWORD*)&r->GPR[op0*2];
    if(iMemToDo & 0x8000) iATAUpdate();
}

//--------------------------------------------------------
// Unaligned Load/Store Helpers
//--------------------------------------------------------
inline void helperLwl(DWORD address,DWORD op0)
{
    DWORD offset = address;
    DWORD data = iMemReadDWord(offset & ~3);
    switch(3-(offset % 4))
    {
        case 0: *(sQWORD *)&r->GPR[op0*2] = (sQWORD)(sDWORD)data; break;
        case 1: *(sQWORD *)&r->GPR[op0*2] = (sQWORD)(sDWORD)((r->GPR[op0*2]&0xFF)| (data<<8)); break;
        case 2: *(sQWORD *)&r->GPR[op0*2] = (sQWORD)(sDWORD)((r->GPR[op0*2]&0xFFFF)| (data<<16)); break;
        case 3: *(sQWORD *)&r->GPR[op0*2] = (sQWORD)(sDWORD)((r->GPR[op0*2]&0xFFFFFF)| (data<<24)); break;
    }
}

inline void helperLwr(DWORD address,DWORD op0)
{
    DWORD offset = address;
    DWORD data = iMemReadDWord(offset & ~3);
    switch(3-(offset % 4))
    {
        case 0: *(sQWORD *)&r->GPR[op0*2] = (*(sQWORD *)&r->GPR[op0*2] & 0xffffff00) | (data>>24); break;
        case 1: *(sQWORD *)&r->GPR[op0*2] = (*(sQWORD *)&r->GPR[op0*2] & 0xffff0000) | (data>>16); break;
        case 2: *(sQWORD *)&r->GPR[op0*2] = (*(sQWORD *)&r->GPR[op0*2] & 0xff000000) | (data>>8); break;
        case 3: *(sQWORD *)&r->GPR[op0*2] = (sQWORD)(sDWORD)data; break;
    }
}

inline void helperLdl(DWORD address,DWORD op0)
{
    DWORD offset = address;
    QWORD data = iMemReadQWord(offset & ~7);
    switch(7-(offset%8))
    {
        case 0: *(QWORD*)&r->GPR[op0*2] = data; break;
        case 1: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xFF) | (data<<8); break;
        case 2: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xFFFF) | (data<<16); break;
        case 3: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xFFFFFF) | (data<<24); break;
        case 4: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xFFFFFFFF) | (data<<32); break;
        case 5: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xFFFFFFFFFF) | (data<<40); break;
        case 6: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xFFFFFFFFFFFF) | (data<<48); break;
        case 7: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xFFFFFFFFFFFFFF) | (data<<56); break;
    }
}

inline void helperLdr(DWORD address,DWORD op0)
{
    DWORD offset = address;
    QWORD data = iMemReadQWord(offset & ~7);
    switch(7-(offset%8))
    {
        case 7: *(QWORD*)&r->GPR[op0*2] = data; break;
        case 6: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xff00000000000000) | (data>>8); break;
        case 5: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xffff000000000000) | (data>>16); break;
        case 4: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xffffff0000000000) | (data>>24); break;
        case 3: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xffffffff00000000) | (data>>32); break;
        case 2: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xffffffffff000000) | (data>>40); break;
        case 1: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xffffffffffff0000) | (data>>48); break;
        case 0: *(QWORD*)&r->GPR[op0*2] = (*(QWORD*)&r->GPR[op0*2]&0xffffffffffffff00) | (data>>56); break;
    }
}

//--------------------------------------------------------
// Store helpers (unaligned swaps)
//--------------------------------------------------------
inline void helperSwl(DWORD offset,DWORD op0)
{
    DWORD old_data = iMemReadDWord(offset & ~3);
    DWORD data;
    switch(3-(offset%4))
    {
        case 0: data = (DWORD)r->GPR[op0*2]; break;
        case 1: data = (old_data & 0xFF000000) | ((DWORD)r->GPR[op0*2]>>8); break;
        case 2: data = (old_data & 0xFFFF0000) | ((DWORD)r->GPR[op0*2]>>16); break;
        case 3: data = (old_data & 0xFFFFFF00) | ((DWORD)r->GPR[op0*2]>>24); break;
        default: data = 0; break;
    }
    iMemWriteDWord(data, offset & ~3);
}

inline void helperSwr(DWORD offset,DWORD op0)
{
    DWORD old_data = iMemReadDWord(offset & ~3);
    DWORD data;
    switch(3-(offset%4))
    {
        case 0: data = (old_data & 0x00ffffff) | ((DWORD)r->GPR[op0*2]<<24); break;
        case 1: data = (old_data & 0x0000ffff) | ((DWORD)r->GPR[op0*2]<<16); break;
        case 2: data = (old_data & 0x000000ff) | ((DWORD)r->GPR[op0*2]<<8); break;
        case 3: data = (DWORD)r->GPR[op0*2]; break;
        default: data = 0; break;
    }
    iMemWriteDWord(data, offset & ~3);
}

inline void helperSdl(DWORD offset,DWORD op0)
{
    QWORD data = iMemReadQWord(offset & ~7);
    switch(7-(offset%8))
    {
        case 0: iMemWriteQWord(*(QWORD*)&r->GPR[op0*2], offset & ~7); break;
        case 1: iMemWriteQWord((data&0xFF00000000000000)|(*(QWORD*)&r->GPR[op0*2]>>8), offset & ~7); break;
        case 2: iMemWriteQWord((data&0xFFFF000000000000)|(*(QWORD*)&r->GPR[op0*2]>>16), offset & ~7); break;
        case 3: iMemWriteQWord((data&0xFFFFFF0000000000)|(*(QWORD*)&r->GPR[op0*2]>>24), offset & ~7); break;
        case 4: iMemWriteQWord((data&0xFFFFFFFF00000000)|(*(QWORD*)&r->GPR[op0*2]>>32), offset & ~7); break;
        case 5: iMemWriteQWord((data&0xFFFFFFFFFF000000)|(*(QWORD*)&r->GPR[op0*2]>>40), offset & ~7); break;
        case 6: iMemWriteQWord((data&0xFFFFFFFFFFFF0000)|(*(QWORD*)&r->GPR[op0*2]>>48), offset & ~7); break;
        case 7: iMemWriteQWord((data&0xFFFFFFFFFFFFFF00)|(*(QWORD*)&r->GPR[op0*2]>>56), offset & ~7); break;
    }
}

inline void helperSdr(DWORD offset,DWORD op0)
{
    QWORD data = iMemReadQWord(offset & ~7);
    switch(7-(offset%8))
    {
        case 7: iMemWriteQWord(*(QWORD*)&r->GPR[op0*2], offset & ~7); break;
        case 6: iMemWriteQWord((data&0xFF)|(*(QWORD*)&r->GPR[op0*2]<<8), offset & ~7); break;
        case 5: iMemWriteQWord((data&0xFFFF)|(*(QWORD*)&r->GPR[op0*2]<<16), offset & ~7); break;
        case 4: iMemWriteQWord((data&0xFFFFFF)|(*(QWORD*)&r->GPR[op0*2]<<24), offset & ~7); break;
        case 3: iMemWriteQWord((data&0xFFFFFFFF)|(*(QWORD*)&r->GPR[op0*2]<<32), offset & ~7); break;
        case 2: iMemWriteQWord((data&0xFFFFFFFFFF)|(*(QWORD*)&r->GPR[op0*2]<<40), offset & ~7); break;
        case 1: iMemWriteQWord((data&0xFFFFFFFFFFFF)|(*(QWORD*)&r->GPR[op0*2]<<48), offset & ~7); break;
        case 0: iMemWriteQWord((data&0xFFFFFFFFFFFFFF)|(*(QWORD*)&r->GPR[op0*2]<<56), offset & ~7); break;
    }
}

//--------------------------------------------------------
// LL/SC helpers
//--------------------------------------------------------
inline void helperLl(DWORD offset,DWORD op0)
{
    *(sQWORD*)&r->GPR[op0*2] = (sQWORD)*(sDWORD*)iMemPhysReadAddr(offset);
    *(sQWORD*)&r->CPR0[2*LLADDR] = (sQWORD)(sDWORD)offset;
    r->Llbit = 1;
}

inline void helperLld(DWORD offset,DWORD op0)
{
    *(sQWORD*)&r->GPR[op0*2] = *(sQWORD*)iMemPhysReadAddr(offset);
    *(sQWORD*)&r->CPR0[2*LLADDR] = (sQWORD)(sDWORD)offset;
    r->Llbit = 1;
}

inline void helperSc(DWORD offset,DWORD op0)
{
    r->Llbit=1;
    if(__builtin_expect(r->Llbit,1))
        iMemWriteDWord((DWORD)r->GPR[op0*2],offset);
    *(sQWORD*)&r->GPR[op0*2] = (sQWORD)(sBYTE)r->Llbit;
}

inline void helperScd(DWORD offset,DWORD op0)
{
    r->Llbit=1;
    if(__builtin_expect(r->Llbit,1))
        iMemWriteQWord(*(QWORD*)&r->GPR[op0*2],offset);
    *(sQWORD*)&r->GPR[op0*2] = (sQWORD)(sBYTE)r->Llbit;
}
