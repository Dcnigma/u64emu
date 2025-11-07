#include <cstdint>
#include <cstring>
#include "iCPU.h"       // Core 4600 emulation routines
#include "iMemory.h"    // Memory emulation routines
#include "iRom.h"       // ROM (cart) emulation routines
#include "hleMain.h"
#include <switch.h>
constexpr int DO_DELAY = 1;
constexpr int NO_DELAY = 0;
constexpr int LINK_OFFSET = 8;

// Optional: enable branch-in-branch checking
#define CHECK_BRANCH_IN_BRANCH

#ifdef CHECK_BRANCH_IN_BRANCH
bool logJAL = false;

inline bool CheckBranchInBranch()
{
    return r->Delay != 0;
}
#endif

// Helper macro to cast for branch operations
#define BRANCH_CAST (*(sDWORD*)&r)

// ================= Branch Instructions =================

void iOpJr()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    r->Delay = DO_DELAY;
    r->PCDelay = static_cast<int32_t>(r->GPR[MAKE_RS * 2]);
}

void iOpJalr()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    r->Delay = DO_DELAY;
    r->PCDelay = static_cast<uint32_t>(r->GPR[MAKE_RS * 2]);
    if((r->PCDelay & 0x7FFFFF) == 0x1EB48)
        logJAL = true;
    if(logJAL)
        LogMessage("JALR %X\n", r->PCDelay);

    r->GPR[MAKE_RD * 2] = static_cast<int64_t>(static_cast<int32_t>(r->PC + LINK_OFFSET));
}

void iOpJ()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(MAKE_T == r->PC - 4)
        r->ICount = r->NextIntCount;

    r->Delay = DO_DELAY;
    r->PCDelay = MAKE_T;
}

void iOpJal()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif

    uint32_t target = MAKE_T;
    if(logJAL)
        LogMessage("JAL %X\n", target);

    // HLE hooks
    if((target & 0x3FFFFF) == 0x30CCC) { hleWriteBlock(); return; }
    if((target & 0x3FFFFF) == 0x3108C) { hleBuildBG(); return; }
    if((target & 0x3FFFFF) == 0x1FC01)
    {
        iCpuDoNextOp();
        char peek[64];
        uint32_t a1 = r->GPR[10];
        int count = *reinterpret_cast<int*>(&m->rdRam[a1 & 0x7FFFFF]);
        while(count > 0)
        {
            a1 += 4;
            uint32_t tmp = *reinterpret_cast<uint32_t*>(&m->rdRam[a1 & 0x7FFFFF]);
            tmp += 6;
            count--;
            std::memcpy(peek, &m->rdRam[tmp & 0x7FFFFF], 63);
        }
    }

    if(target == 0x88029F85)
    {
        uint32_t delay  = *reinterpret_cast<uint32_t*>(&m->rdRam[0x8F5B4]);
        uint32_t start  = *reinterpret_cast<uint32_t*>(&m->rdRam[0x8F5BC]);
        r->ICount = start + delay;
        r->PC += 4;
    }
    else
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_T;
        r->GPR[31 * 2] = static_cast<int64_t>(static_cast<int32_t>(r->PC + LINK_OFFSET));
    }
}

// ================= Conditional Branches =================

void iOpBeq()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] == BRANCH_CAST.GPR[MAKE_RT * 2])
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
        if(r->PCDelay == r->PC - 4) r->ICount = r->NextIntCount;
    }
}

void iOpBne()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] != BRANCH_CAST.GPR[MAKE_RT * 2])
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
        if(r->PCDelay == r->PC - 4) r->ICount = r->NextIntCount;
    }
}

void iOpBlez()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] <= 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
}

void iOpBgtz()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] > 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
}

// ================= Likely Branches =================

void iOpBeql()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] == BRANCH_CAST.GPR[MAKE_RT * 2])
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else r->PC += 4;
}

void iOpBnel()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] != BRANCH_CAST.GPR[MAKE_RT * 2])
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else r->PC += 4;
}

// ================= Other Branch Instructions =================

void iOpEret()
{
    if(r->CPR0[STATUS * 2] & 0x0004)  // ERL exception
    {
        r->PC = static_cast<uint32_t>(r->CPR0[2 * ERROREPC] - 4);
        r->CPR0[2 * STATUS] &= 0xFFFFFFFFFFFFFFFBULL;
    }
    else  // normal exception
    {
        r->PC = static_cast<uint32_t>(r->CPR0[2 * EPC]);
        r->CPR0[2 * STATUS] &= 0xFFFFFFFFFFFFFFFDULL;
    }
    r->PC |= 0x88000000;
    r->Delay = NO_DELAY;
    r->Llbit = 0;
}

// ================= Bltz/Bgez Branches =================

void iOpBltz()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] < 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
}

void iOpBgez()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] >= 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
}

void iOpBltzl()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] < 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else
        r->PC += 4;
}

void iOpBgezl()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(BRANCH_CAST.GPR[MAKE_RS * 2] >= 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else
        r->PC += 4;
}

void iOpBltzal()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    r->GPR[31 * 2] = static_cast<int32_t>(r->PC + LINK_OFFSET);
    if(BRANCH_CAST.GPR[MAKE_RS * 2] < 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
}

void iOpBgezal()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    r->GPR[31 * 2] = static_cast<int32_t>(r->PC + LINK_OFFSET);
    if(BRANCH_CAST.GPR[MAKE_RS * 2] >= 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else
        r->PC += 4;
}

void iOpBltzall()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    r->GPR[31 * 2] = static_cast<int32_t>(r->PC + LINK_OFFSET);
    if(BRANCH_CAST.GPR[MAKE_RS * 2] < 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else
        r->PC += 4;
}

void iOpBgezall()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    r->GPR[31 * 2] = static_cast<int32_t>(r->PC + LINK_OFFSET);
    if(BRANCH_CAST.GPR[MAKE_RS * 2] >= 0)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else
        r->PC += 4;
}

// ================= CCR1 Conditional Branches =================

void iOpBcf()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(!(r->CCR1[2 * 31] & 0x00800000))
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
}

void iOpBct()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(r->CCR1[2 * 31] & 0x00800000)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
}

void iOpBcfl()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(!(r->CCR1[2 * 31] & 0x00800000))
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else
        r->PC += 4;
}

void iOpBctl()
{
#ifdef CHECK_BRANCH_IN_BRANCH
    if(CheckBranchInBranch()) return;
#endif
    if(r->CCR1[2 * 31] & 0x00800000)
    {
        r->Delay = DO_DELAY;
        r->PCDelay = MAKE_O;
    }
    else
        r->PC += 4;
}
