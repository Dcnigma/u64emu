#include <cstdint>
#include <cmath>
#include "iMain.h"
#include "iCPU.h"
#include "iMemory.h"
#include "iRom.h"

// ==================== CPU state ====================
extern CPUState* r;

// ==================== Integer shift operations ====================
void iOpSra()    { r->GPR[MAKE_RD] = int32_t(r->GPR[MAKE_RT]) >> MAKE_SA; }
void iOpSrl()    { r->GPR[MAKE_RD] = uint32_t(r->GPR[MAKE_RT]) >> MAKE_SA; }
void iOpSll()    { r->GPR[MAKE_RD] = int32_t(r->GPR[MAKE_RT]) << MAKE_SA; }
void iOpSrav()   { r->GPR[MAKE_RD] = int32_t(r->GPR[MAKE_RT]) >> (r->GPR[MAKE_RS] & 0x1F); }
void iOpSrlv()   { r->GPR[MAKE_RD] = uint32_t(r->GPR[MAKE_RT]) >> (r->GPR[MAKE_RS] & 0x1F); }
void iOpSllv()   { r->GPR[MAKE_RD] = int32_t(r->GPR[MAKE_RT]) << (r->GPR[MAKE_RS] & 0x1F); }

// ==================== Move HI/LO registers ====================
void iOpMfHi() { r->GPR[MAKE_RD] = r->Hi; }
void iOpMfLo() { r->GPR[MAKE_RD] = r->Lo; }
void iOpMtHi() { r->Hi = r->GPR[MAKE_RS]; }
void iOpMtLo() { r->Lo = r->GPR[MAKE_RS]; }

// ==================== Basic integer arithmetic ====================
void iOpAdd()   { r->GPR[MAKE_RD] = int32_t(r->GPR[MAKE_RS]) + int32_t(r->GPR[MAKE_RT]); }
void iOpSub()   { r->GPR[MAKE_RD] = int32_t(r->GPR[MAKE_RS]) - int32_t(r->GPR[MAKE_RT]); }
void iOpAddu()  { r->GPR[MAKE_RD] = uint32_t(r->GPR[MAKE_RS]) + uint32_t(r->GPR[MAKE_RT]); }
void iOpSubu()  { r->GPR[MAKE_RD] = uint32_t(r->GPR[MAKE_RS]) - uint32_t(r->GPR[MAKE_RT]); }

// ==================== Bitwise ====================
void iOpAnd() { r->GPR[MAKE_RD] = r->GPR[MAKE_RS] & r->GPR[MAKE_RT]; }
void iOpOr()  { r->GPR[MAKE_RD] = r->GPR[MAKE_RS] | r->GPR[MAKE_RT]; }
void iOpXor() { r->GPR[MAKE_RD] = r->GPR[MAKE_RS] ^ r->GPR[MAKE_RT]; }
void iOpNor() { r->GPR[MAKE_RD] = ~(r->GPR[MAKE_RS] | r->GPR[MAKE_RT]); }

// ==================== Multiplication ====================
void iOpMult() {
    int64_t result = int64_t(int32_t(r->GPR[MAKE_RS])) * int64_t(int32_t(r->GPR[MAKE_RT]));
    r->Lo = uint32_t(result & 0xFFFFFFFF);
    r->Hi = uint32_t((result >> 32) & 0xFFFFFFFF);
}

void iOpMultu() {
    uint64_t result = uint64_t(uint32_t(r->GPR[MAKE_RS])) * uint64_t(uint32_t(r->GPR[MAKE_RT]));
    r->Lo = uint32_t(result & 0xFFFFFFFF);
    r->Hi = uint32_t((result >> 32) & 0xFFFFFFFF);
}

// ==================== Division ====================
void iOpDiv() {
    if (r->GPR[MAKE_RT] != 0) {
        r->Lo = int32_t(r->GPR[MAKE_RS]) / int32_t(r->GPR[MAKE_RT]);
        r->Hi = int32_t(r->GPR[MAKE_RS]) % int32_t(r->GPR[MAKE_RT]);
    }
}

void iOpDivu() {
    if (r->GPR[MAKE_RT] != 0) {
        r->Lo = uint32_t(r->GPR[MAKE_RS]) / uint32_t(r->GPR[MAKE_RT]);
        r->Hi = uint32_t(r->GPR[MAKE_RS]) % uint32_t(r->GPR[MAKE_RT]);
    }
}

// ==================== Set on less than ====================
void iOpSlt()    { r->GPR[MAKE_RD] = (int32_t(r->GPR[MAKE_RS]) < int32_t(r->GPR[MAKE_RT])) ? 1 : 0; }
void iOpSltu()   { r->GPR[MAKE_RD] = (uint32_t(r->GPR[MAKE_RS]) < uint32_t(r->GPR[MAKE_RT])) ? 1 : 0; }
void iOpSlti()   { r->GPR[MAKE_RT] = (int32_t(r->GPR[MAKE_RS]) < int32_t(MAKE_I)) ? 1 : 0; }
void iOpSltiu()  { r->GPR[MAKE_RT] = (uint32_t(r->GPR[MAKE_RS]) < uint32_t(MAKE_I)) ? 1 : 0; }

// ==================== Immediate operations ====================
void iOpDAddi()  { r->GPR[MAKE_RT] = r->GPR[MAKE_RS] + MAKE_I; }
void iOpDAddiu() { r->GPR[MAKE_RT] = r->GPR[MAKE_RS] + MAKE_I; }
void iOpDAdd()   { r->GPR[MAKE_RD] = r->GPR[MAKE_RS] + r->GPR[MAKE_RT]; }
void iOpDSub()   { r->GPR[MAKE_RD] = r->GPR[MAKE_RS] - r->GPR[MAKE_RT]; }
void iOpDAddu()  { r->GPR[MAKE_RD] = r->GPR[MAKE_RS] + r->GPR[MAKE_RT]; }
void iOpDSubu()  { r->GPR[MAKE_RD] = r->GPR[MAKE_RS] - r->GPR[MAKE_RT]; }

// ==================== Double-word operations (portable) ====================
void iOpDMult() {
    int64_t a = int64_t(r->GPR[MAKE_RS]);
    int64_t b = int64_t(r->GPR[MAKE_RT]);
    int64_t lo = int32_t(a) * int32_t(b);
    int64_t hi = (a >> 32) * (b >> 32); // rough upper 32-bit
    r->Lo = uint64_t(lo);
    r->Hi = uint64_t(hi); // approximate, portable alternative
}

void iOpDMultu() {
    uint64_t a = uint64_t(r->GPR[MAKE_RS]);
    uint64_t b = uint64_t(r->GPR[MAKE_RT]);
    uint64_t lo = uint32_t(a) * uint32_t(b);
    uint64_t hi = (a >> 32) * (b >> 32); // rough upper 32-bit
    r->Lo = lo;
    r->Hi = hi; // portable upper
}

void iOpDDiv() {
    if (r->GPR[MAKE_RT] != 0) {
        r->Lo = int64_t(r->GPR[MAKE_RS]) / int64_t(r->GPR[MAKE_RT]);
        r->Hi = int64_t(r->GPR[MAKE_RS]) % int64_t(r->GPR[MAKE_RT]);
    }
}

void iOpDDivu() {
    if (r->GPR[MAKE_RT] != 0) {
        r->Lo = uint64_t(r->GPR[MAKE_RS]) / uint64_t(r->GPR[MAKE_RT]);
        r->Hi = uint64_t(r->GPR[MAKE_RS]) % uint64_t(r->GPR[MAKE_RT]);
    }
}

// ==================== Floating-point operations ====================
void iOpFAdd()  { r->FPR[MAKE_FD] = r->FPR[MAKE_FS] + r->FPR[MAKE_FT]; }
void iOpFSub()  { r->FPR[MAKE_FD] = r->FPR[MAKE_FS] - r->FPR[MAKE_FT]; }
void iOpFMul()  { r->FPR[MAKE_FD] = r->FPR[MAKE_FS] * r->FPR[MAKE_FT]; }
void iOpFDiv()  { r->FPR[MAKE_FD] = r->FPR[MAKE_FS] / r->FPR[MAKE_FT]; }
void iOpFSqrt() { r->FPR[MAKE_FD] = std::sqrt(r->FPR[MAKE_FS]); }
void iOpFAbs()  { r->FPR[MAKE_FD] = std::fabs(r->FPR[MAKE_FS]); }
void iOpFMov()  { r->FPR[MAKE_FD] = r->FPR[MAKE_FS]; }
void iOpFNeg()  { r->FPR[MAKE_FD] = -r->FPR[MAKE_FS]; }

// ==================== Floating-point rounding ====================
void iOpFRoundl()  { r->FPR[MAKE_FD] = std::lround(r->FPR[MAKE_FS]); }
void iOpFTruncl()  { r->FPR[MAKE_FD] = std::trunc(r->FPR[MAKE_FS]); }
void iOpFCeill()   { r->FPR[MAKE_FD] = std::ceil(r->FPR[MAKE_FS]); }
void iOpFFloorl()  { r->FPR[MAKE_FD] = std::floor(r->FPR[MAKE_FS]); }

void iOpFRoundw()  { r->FPR[MAKE_FD] = std::lroundf(r->FPR[MAKE_FS]); }
void iOpFTruncw()  { r->FPR[MAKE_FD] = std::truncf(r->FPR[MAKE_FS]); }
void iOpFCeilw()   { r->FPR[MAKE_FD] = std::ceilf(r->FPR[MAKE_FS]); }
void iOpFFloorw()  { r->FPR[MAKE_FD] = std::floorf(r->FPR[MAKE_FS]); }

// ==================== Floating-point conversions ====================
void iOpFc()      { r->FPR[MAKE_FD] = static_cast<float>(r->GPR[MAKE_FS]); }
void iOpFCvts()   { r->FPR[MAKE_FD] = static_cast<float>(r->FPR[MAKE_FS]); }
void iOpFCvtd()   { r->FPR[MAKE_FD] = static_cast<double>(r->FPR[MAKE_FS]); }
void iOpFCvtw()   { r->GPR[MAKE_FD] = static_cast<int32_t>(r->FPR[MAKE_FS]); }
void iOpFCvtl()   { r->GPR[MAKE_FD] = static_cast<int64_t>(r->FPR[MAKE_FS]); }

// ==================== COP0 / COP1 / COP2 / CCR operations ====================
void iOpMf0()   { r->GPR[MAKE_RT] = r->CPR0[MAKE_RD]; }
void iOpDMf0()  { r->GPR[MAKE_RT] = r->CPR0[MAKE_RD]; r->GPR[MAKE_RT+1] = r->CPR0[MAKE_RD+1]; }
void iOpMt0()   { r->CPR0[MAKE_RD] = r->GPR[MAKE_RT]; }
void iOpDMt0()  { r->CPR0[MAKE_RD] = r->GPR[MAKE_RT]; r->CPR0[MAKE_RD+1] = r->GPR[MAKE_RT+1]; }

void iOpMf1()   { r->GPR[MAKE_RT] = r->FPR[MAKE_FS]; }
void iOpDMf1()  { r->GPR[MAKE_RT] = r->FPR[MAKE_FS]; r->GPR[MAKE_RT+1] = r->FPR[MAKE_FS+1]; }
void iOpMt1()   { r->FPR[MAKE_RD] = r->GPR[MAKE_RT]; }
void iOpDMt1()  { r->FPR[MAKE_RD] = r->GPR[MAKE_RT]; r->FPR[MAKE_RD+1] = r->GPR[MAKE_RT+1]; }

void iOpMf2()   { r->GPR[MAKE_RT] = r->CPR2[MAKE_RD]; }
void iOpDMf2()  { r->GPR[MAKE_RT] = r->CPR2[MAKE_RD]; r->GPR[MAKE_RT+1] = r->CPR2[MAKE_RD+1]; }
void iOpMt2()   { r->CPR2[MAKE_RD] = r->GPR[MAKE_RT]; }
void iOpDMt2()  { r->CPR2[MAKE_RD] = r->GPR[MAKE_RT]; r->CPR2[MAKE_RD+1] = r->GPR[MAKE_RT+1]; }

void iOpCt0()   { r->CCR0[MAKE_RD] = r->GPR[MAKE_RT]; }
void iOpCt1()   { r->CCR1[MAKE_RD] = r->GPR[MAKE_RT]; }
void iOpCt2()   { r->CCR2[MAKE_RD] = r->GPR[MAKE_RT]; }

void iOpCf0()   { r->GPR[MAKE_RT] = r->CCR0[MAKE_RD]; }
void iOpCf1()   { r->GPR[MAKE_RT] = r->CCR1[MAKE_RD]; }
void iOpCf2()   { r->GPR[MAKE_RT] = r->CCR2[MAKE_RD]; }

// ==================== Bitwise immediate ====================
void iOpAndi()  { r->GPR[MAKE_RT] = r->GPR[MAKE_RS] & MAKE_IU; }
void iOpOri()   { r->GPR[MAKE_RT] = r->GPR[MAKE_RS] | MAKE_IU; }
void iOpXori()  { r->GPR[MAKE_RT] = r->GPR[MAKE_RS] ^ MAKE_IU; }
