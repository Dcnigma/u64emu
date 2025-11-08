// N64Mem.h
#ifndef N64MEM_H
#define N64MEM_H

#include <cstdint>

typedef struct N64Mem {
    uint8_t *rdRam;
    uint8_t *spDmem;
    uint8_t *spImem;
    uint8_t *piRom;
    uint8_t *piRam;
    uint8_t *piRamW;

    uint8_t *rdReg;
    uint8_t *spReg;
    uint8_t *dpcReg;
    uint8_t *dpsReg;
    uint8_t *miReg;
    uint8_t *miRegW;
    uint8_t *viReg;
    uint8_t *aiReg;
    uint8_t *piReg;
    uint8_t *piRegW;
    uint8_t *riReg;
    uint8_t *siReg;
    uint8_t *NullMem;
    uint8_t *atReg;
    uint8_t *dspPMem;
    uint8_t *dspDMem;
    uint8_t *dspRMem;

    uint32_t miRegModeRO;
    uint32_t miRegIntrMaskRO;

    uint32_t spRegPC;
    uint32_t spRegIbist;
    uint32_t spRegStatusRO;
    uint32_t dpcRegStatusRO;
    uint32_t piRegStatusRO;
} N64Mem;

#endif // N64MEM_H
