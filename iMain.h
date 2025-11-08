#ifndef IMAIN_H
#define IMAIN_H

#include <cstdint>
#include "N64Mem.h"
#define NO_DELAY        0
#define DO_DELAY        1
#define EXEC_DELAY      2
#define RELOAD_PC       3

#define LINK_OFFSET 4

#define INC_AMOUNT  625000*2
#define VTRACE_TIME 625000*2

#define EXC_CODE__MASK  0x7c

#define BOOT_STAGE0 0
#define NORMAL_GAME 1
#define SAVE_GAME 2
#define LOAD_GAME 3
#define EXIT_EMU 4
#define HALT_GAME 5
#define RESUME_GAME 6
#define GAME_HALTED 7
#define DEBUG_BREAK 8
#define DEBUG_RESUME 9
#define DEBUG_STEPMODE 10
#define DEBUG_SINGLESTEP 11
#define ERROR_BREAK 12
#define DEBUG_FASTSTEP 13

#define MI_INTR_NO  0x00
#define MI_INTR_ALL 0x3f
#define MI_INTR_SP  0x01
#define MI_INTR_SI  0x02
#define MI_INTR_AI  0x04
#define MI_INTR_VI  0x08
#define MI_INTR_PI  0x10
#define MI_INTR_DP  0x20

typedef struct N64RomInfoStruct {
    char  *real_name;
    char  *dmem_name;
    char  *imem_name;

    uint16_t Validation;       /* 0x00 */
    uint8_t  Compression;      /* 0x02 */
    uint8_t  Unknown1;         /* 0x03 */
    uint32_t Clockrate;        /* 0x04 */
    uint32_t ProgramCounter;   /* 0x08 */
    uint32_t Release;          /* 0x0c */

    uint32_t Crc1;             /* 0x10 */
    uint32_t Crc2;             /* 0x14 */
    uint32_t Unknown2;          /* 0x18 */
    uint32_t Unknown2a;         /* 0x1c */

    uint8_t  Name[20];         /* 0x20 - 0x33 */

    uint8_t  Unknown3;         /* 0x34 */
    uint8_t  Unknown4;         /* 0x35 */
    uint8_t  Unknown5;         /* 0x36 */
    uint8_t  Unknown6;         /* 0x37 */
    uint8_t  Unknown7;         /* 0x38 */
    uint8_t  Unknown8;         /* 0x39 */
    uint8_t  Unknown9;         /* 0x3a */
    uint8_t  ManufacturerId;   /* 0x3b */
    uint16_t CartridgeId;      /* 0x3c */
    uint8_t  CountryCode;      /* 0x3e */
    uint8_t  Unknown10;        /* 0x3f */
} N64RomInfoStruct;

typedef struct RS4300iTlb {
    uint32_t hh;
    uint32_t hl;
    uint32_t lh;
    uint32_t ll;
    bool g;
} RS4300iTlb;

typedef struct N64RomStruct {
    uint8_t    *Image;
    long       Length;

    uint8_t    *Header;
    uint8_t    *BootCode;
    uint32_t   *PrgCode;

    uint32_t   PrgCodeBaseOrig;
    uint32_t   PrgCodeBase;
    long       PrgCodeLength;
    N64RomInfoStruct Info;
} N64RomStruct;

typedef struct RS4300iReg {
    int32_t GPR[64];   // General purpose registers

    int32_t CPR0[64];
    int32_t CPR1[64];
    int32_t CPR2[64];

    int32_t CCR0[64];
    int32_t CCR1[64];
    int32_t CCR2[64];

    int32_t FPR[32];

    uint64_t Lo, Hi;

    uint32_t PC;

    uint32_t PCDelay;
    int      Delay;

    int      DoOrCheckSthg;

    uint32_t CurRoundMode;
    uint32_t Code;

    uint32_t LastPC;
    uint32_t Break;
    bool     Llbit;
    uint8_t  pad[3];
    uint64_t ICount;
    uint32_t CompareCount;
    uint64_t NextIntCount;
    uint64_t VTraceCount;
    uint32_t RoundMode;
    uint32_t TruncMode;
    uint32_t CeilMode;
    uint32_t FloorMode;
    RS4300iTlb Tlb[48];
} RS4300iReg;



extern volatile uint16_t NewTask;
extern volatile uint16_t DspTask;

extern uint16_t iFPUMode;
extern RS4300iReg *r;
extern N64Mem *m;
extern N64RomStruct *rom;

extern uint32_t iOpCode;
extern uint32_t iNextOpCode;
extern uint32_t iPC;

// CPU register index defines
#define INDEX           0
#define RANDOM          1
#define ENTRYLO0        2
#define ENTRYLO1        3
#define CONTEXT         4
#define PAGEMASK        5
#define WIRED           6
#define BADVADDR        8
#define COUNT           9
#define ENTRYHI         10
#define COMPARE         11
#define STATUS          12
#define CAUSE           13
#define EPC             14
#define PRID            15
#define CONFIG          16
#define LLADDR          17
#define WATCHLO         18
#define WATCHHI         19
#define XCONTEXT        20
#define ECC             26
#define CACHEERR        27
#define TAGLO           28
#define TAGHI           29
#define ERROREPC        30

// OpCode helpers
#define MAKE_RS           (((uint8_t)(iOpCode >> 21)) & 0x1f)
#define MAKE_RT           (((uint8_t)(iOpCode >> 16)) & 0x1f)
#define MAKE_RD           (((uint8_t)(iOpCode >> 11)) & 0x1f)
#define MAKE_SA           (((uint8_t)(iOpCode >>  6)) & 0x1f)
#define MAKE_F            ((uint8_t)(iOpCode) & 0x3f)
#define MAKE_I            ((int16_t)(iOpCode & 0xffff))
#define MAKE_IU           ((uint16_t)(iOpCode & 0xffff))
#define MAKE_VD           (((uint8_t)(iOpCode >> 6)) & 0x1f)
#define MAKE_VS1          (((uint8_t)(iOpCode >> 11)) & 0x1f)
#define MAKE_VS2          (((uint8_t)(iOpCode >> 16)) & 0x1f)
#define MAKE_VSEL         (((uint8_t)(iOpCode >> 21)) & 0xf)

#define MAKE_FMT MAKE_RS
#define MAKE_FT  MAKE_RT
#define MAKE_FS  MAKE_RD
#define MAKE_FD  MAKE_SA

#define FMT_S   16
#define FMT_D   17
#define FMT_W   20
#define FMT_L   21

#define MAKE_O    ( r->PC + (MAKE_I << 2) )
#define ____T     (iOpCode & 0x00ffffff)
#define MAKE_T    (((r->PC & 0xff000000) | (____T << 2)))

// Function declarations
extern void iMainConstruct(const char *filename);
extern void iMainDestruct();
extern void iMainThreadProc();
extern void iMainReset();
extern void iMainStopCPU();
extern void iMainStartCPU();
extern void iMainResetDSP();

#endif
