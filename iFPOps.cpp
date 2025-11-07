#include <cmath>
#include <cstdint>
#include "iMain.h"
#include "iCPU.h"
#include "iMemory.h"
#include "iRom.h"

// ==================== Floating Point Access Helpers ====================
#define SINGLE_STORE reinterpret_cast<float&>(r->FPR[MAKE_FD])
#define DOUBLE_STORE reinterpret_cast<double&>(r->FPR[MAKE_FD])

// ==================== Floating Point Arithmetic ====================
void iOpFAdd() {
    if(MAKE_FMT == FMT_S)
        SINGLE_STORE = reinterpret_cast<float&>(r->FPR[MAKE_FS]) + reinterpret_cast<float&>(r->FPR[MAKE_FT]);
    else
        DOUBLE_STORE = reinterpret_cast<double&>(r->FPR[MAKE_FS]) + reinterpret_cast<double&>(r->FPR[MAKE_FT]);
}

void iOpFSub() {
    if(MAKE_FMT == FMT_S)
        SINGLE_STORE = reinterpret_cast<float&>(r->FPR[MAKE_FS]) - reinterpret_cast<float&>(r->FPR[MAKE_FT]);
    else
        DOUBLE_STORE = reinterpret_cast<double&>(r->FPR[MAKE_FS]) - reinterpret_cast<double&>(r->FPR[MAKE_FT]);
}

void iOpFMul() {
    if(MAKE_FMT == FMT_S)
        SINGLE_STORE = reinterpret_cast<float&>(r->FPR[MAKE_FS]) * reinterpret_cast<float&>(r->FPR[MAKE_FT]);
    else
        DOUBLE_STORE = reinterpret_cast<double&>(r->FPR[MAKE_FS]) * reinterpret_cast<double&>(r->FPR[MAKE_FT]);
}

void iOpFDiv() {
    if(MAKE_FMT == FMT_S) {
        float ft = reinterpret_cast<float&>(r->FPR[MAKE_FT]);
        if(ft != 0.f) SINGLE_STORE = reinterpret_cast<float&>(r->FPR[MAKE_FS]) / ft;
    } else {
        double ft = reinterpret_cast<double&>(r->FPR[MAKE_FT]);
        if(ft != 0.0) DOUBLE_STORE = reinterpret_cast<double&>(r->FPR[MAKE_FS]) / ft;
    }
}

void iOpFSqrt() {
    if(MAKE_FMT == FMT_S)
        SINGLE_STORE = std::sqrt(reinterpret_cast<float&>(r->FPR[MAKE_FS]));
    else
        DOUBLE_STORE = std::sqrt(reinterpret_cast<double&>(r->FPR[MAKE_FS]));
}

void iOpFAbs() {
    if(MAKE_FMT == FMT_S)
        SINGLE_STORE = std::fabs(reinterpret_cast<float&>(r->FPR[MAKE_FS]));
    else
        DOUBLE_STORE = std::fabs(reinterpret_cast<double&>(r->FPR[MAKE_FS]));
}

void iOpFNeg() {
    if(MAKE_FMT == FMT_S)
        SINGLE_STORE = -reinterpret_cast<float&>(r->FPR[MAKE_FS]);
    else
        DOUBLE_STORE = -reinterpret_cast<double&>(r->FPR[MAKE_FS]);
}

void iOpFMov() {
    if(MAKE_FMT == FMT_S)
        r->FPR[MAKE_FD] = r->FPR[MAKE_FS];
    else {
        r->FPR[MAKE_FD] = r->FPR[MAKE_FS];
        r->FPR[MAKE_FD+1] = r->FPR[MAKE_FS+1];
    }
}

// ==================== Floating Point Rounding ====================
void iOpFRoundl() {
    if(MAKE_FMT == FMT_S)
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(reinterpret_cast<float&>(r->FPR[MAKE_FS]));
    else
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(reinterpret_cast<double&>(r->FPR[MAKE_FS]));
}

void iOpFTruncl() {
    if(MAKE_FMT == FMT_S)
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(std::trunc(reinterpret_cast<float&>(r->FPR[MAKE_FS])));
    else
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(std::trunc(reinterpret_cast<double&>(r->FPR[MAKE_FS])));
}

void iOpFCeill() {
    if(MAKE_FMT == FMT_S)
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(std::ceil(reinterpret_cast<float&>(r->FPR[MAKE_FS])));
    else
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(std::ceil(reinterpret_cast<double&>(r->FPR[MAKE_FS])));
}

void iOpFFloorl() {
    if(MAKE_FMT == FMT_S)
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(std::floor(reinterpret_cast<float&>(r->FPR[MAKE_FS])));
    else
        *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(std::floor(reinterpret_cast<double&>(r->FPR[MAKE_FS])));
}

void iOpFRoundw() {
    if(MAKE_FMT == FMT_S)
        r->FPR[MAKE_FD] = static_cast<int32_t>(reinterpret_cast<float&>(r->FPR[MAKE_FS]));
    else
        r->FPR[MAKE_FD] = static_cast<int32_t>(reinterpret_cast<double&>(r->FPR[MAKE_FS]));
}

void iOpFTruncw() {
    if(MAKE_FMT == FMT_S)
        r->FPR[MAKE_FD] = static_cast<int32_t>(std::trunc(reinterpret_cast<float&>(r->FPR[MAKE_FS])));
    else
        r->FPR[MAKE_FD] = static_cast<int32_t>(std::trunc(reinterpret_cast<double&>(r->FPR[MAKE_FS])));
}

void iOpFCeilw() {
    if(MAKE_FMT == FMT_S)
        r->FPR[MAKE_FD] = static_cast<int32_t>(std::ceil(reinterpret_cast<float&>(r->FPR[MAKE_FS])));
    else
        r->FPR[MAKE_FD] = static_cast<int32_t>(std::ceil(reinterpret_cast<double&>(r->FPR[MAKE_FS])));
}

void iOpFFloorw() {
    if(MAKE_FMT == FMT_S)
        r->FPR[MAKE_FD] = static_cast<int32_t>(std::floor(reinterpret_cast<float&>(r->FPR[MAKE_FS])));
    else
        r->FPR[MAKE_FD] = static_cast<int32_t>(std::floor(reinterpret_cast<double&>(r->FPR[MAKE_FS])));
}

// ==================== Conversion Operations ====================
void iOpFc() {
    uint64_t& CCRbit = reinterpret_cast<uint64_t&>(r->CCR1[31*2]);
    float fs_s = reinterpret_cast<float&>(r->FPR[MAKE_FS]);
    float ft_s = reinterpret_cast<float&>(r->FPR[MAKE_FT]);
    double fs_d = reinterpret_cast<double&>(r->FPR[MAKE_FS]);
    double ft_d = reinterpret_cast<double&>(r->FPR[MAKE_FT]);

    switch(MAKE_FMT) {
        case FMT_S:
            switch(MAKE_F) {
                case 0x30: CCRbit &= ~0x00800000; break; // c.f
                case 0x31: CCRbit &= ~0x00800000; break; // c.un
                case 0x32: CCRbit = (fs_s == ft_s) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x33: CCRbit = (fs_s == ft_s) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3c: CCRbit = (fs_s < ft_s) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3d: CCRbit = (fs_s < ft_s) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3e: CCRbit = (fs_s <= ft_s) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3f: CCRbit = (fs_s <= ft_s) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
            }
            break;
        case FMT_D:
            switch(MAKE_F) {
                case 0x30: CCRbit &= ~0x00800000; break;
                case 0x31: CCRbit &= ~0x00800000; break;
                case 0x32: CCRbit = (fs_d == ft_d) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x33: CCRbit = (fs_d == ft_d) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3c: CCRbit = (fs_d < ft_d) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3d: CCRbit = (fs_d < ft_d) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3e: CCRbit = (fs_d <= ft_d) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
                case 0x3f: CCRbit = (fs_d <= ft_d) ? CCRbit | 0x00800000 : CCRbit & ~0x00800000; break;
            }
            break;
        default:
            break;
    }
}

void iOpFCvts() {
    switch(MAKE_FMT) {
        case FMT_S: SINGLE_STORE = reinterpret_cast<float&>(r->FPR[MAKE_FS]); break;
        case FMT_D: SINGLE_STORE = static_cast<float>(reinterpret_cast<double&>(r->FPR[MAKE_FS])); break;
        case FMT_W: SINGLE_STORE = static_cast<float>(r->FPR[MAKE_FS]); break;
        case FMT_L: SINGLE_STORE = static_cast<float>(*reinterpret_cast<int64_t*>(&r->FPR[MAKE_FS])); break;
    }
}

void iOpFCvtd() {
    switch(MAKE_FMT) {
        case FMT_S: DOUBLE_STORE = static_cast<double>(reinterpret_cast<float&>(r->FPR[MAKE_FS])); break;
        case FMT_D: DOUBLE_STORE = reinterpret_cast<double&>(r->FPR[MAKE_FS]); break;
        case FMT_W: DOUBLE_STORE = static_cast<double>(r->FPR[MAKE_FS]); break;
        case FMT_L: DOUBLE_STORE = static_cast<double>(*reinterpret_cast<int64_t*>(&r->FPR[MAKE_FS])); break;
    }
}

void iOpFCvtw() {
    switch(MAKE_FMT) {
        case FMT_S: r->FPR[MAKE_FD] = static_cast<int32_t>(reinterpret_cast<float&>(r->FPR[MAKE_FS])); break;
        case FMT_D: r->FPR[MAKE_FD] = static_cast<int32_t>(reinterpret_cast<double&>(r->FPR[MAKE_FS])); break;
        case FMT_W: r->FPR[MAKE_FD] = r->FPR[MAKE_FS]; break;
        case FMT_L: r->FPR[MAKE_FD] = static_cast<int32_t>(*reinterpret_cast<int64_t*>(&r->FPR[MAKE_FS])); break;
    }
}

void iOpFCvtl() {
    switch(MAKE_FMT) {
        case FMT_S: *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(reinterpret_cast<float&>(r->FPR[MAKE_FS])); break;
        case FMT_D: *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(reinterpret_cast<double&>(r->FPR[MAKE_FS])); break;
        case FMT_W: *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = static_cast<int64_t>(r->FPR[MAKE_FS]); break;
        case FMT_L: *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FD]) = *reinterpret_cast<int64_t*>(&r->FPR[MAKE_FS]); break;
    }
}
