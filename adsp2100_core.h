#ifndef _ADSP2100_CORE_H
#define _ADSP2100_CORE_H

#include <cstdint>

#define PC_STACK_DEPTH 8

// Core registers
struct ADSPCORE {
    struct { int32_t s; } ax0, ax1;
    struct { int32_t s; } ay0, ay1;
    struct { int32_t s; } ar, af;
    struct { int32_t s; } mx0, mx1;
    struct { int32_t s; } my0, my1;
    struct { int32_t s; } mr0, mr1, mr2, mf;
    struct { int32_t s; } si, se, sb, sr0, sr1;
    struct { int32_t s; } i[8];
    struct { int32_t s; } l[8];
    struct { int32_t s; } m[8];
    struct { int32_t s; } px;
};

// Full CPU state (core + PC, stack, flags, timers)
struct ADSP2100 {
    // CPU state
    uint16_t pc;
    uint16_t pc_stack[PC_STACK_DEPTH];
    uint8_t  pc_sp;
    uint16_t astat;
    uint16_t astat_clear;
    uint16_t sstat;
    uint16_t mstat;
    uint16_t loop_addr;
    uint16_t lc;

    // I/O and timers
    int16_t timer_control;
    int16_t serial_data;

    // Core
    ADSPCORE core;
};

// Global
extern ADSP2100 adsp2100;

#endif // _ADSP2100_CORE_H
