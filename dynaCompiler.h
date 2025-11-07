#ifndef DYNA_COMPILER_H
#define DYNA_COMPILER_H

#include <stdint.h>
#include <stdbool.h>

// Memory configuration
#define MEM_MASK    0x7FFFFF
#define MEM_SIZE    0x800000

#define PAGE_SIZE   0x8000
#define NUM_PAGES   4
#define PAGE_MASK   0x00060000
#define OFFSET_MASK 0x0001FFFC
#define PAGE_SHIFT  17
#define OFFSET_SHIFT 2

// CPU/ILoop flags
#define GPR0_OPTIMIZATION
#define RD_EQUAL_RS
#define BASIC_ILOOP
#define VARI_ILOOPn
#define ALL_ILOOPn

// Native register abstraction (for ARM64)
#define NATIVE_REGS 4   // pseudo-registers used for dynamic compilation

// Register mapping offsets
#define GPR_     0
#define CPR0_    (GPR_ + 256)
#define CPR1_    (CPR0_ + 256)
#define CPR2_    (CPR1_ + 256)
#define CCR0_    (CPR2_ + 256)
#define CCR1_    (CCR0_ + 256)
#define CCR2_    (CCR1_ + 256)
#define _FPR     (CCR2_ + 256)
#define LO       (_FPR + 128)
#define HI       (LO + 8)
#define PC_      (HI + 8)
#define PCDELAY  (PC_ + 4)
#define DELAY    (PCDELAY + 4)
#define DORC     (DELAY + 4)
#define CUR_ROUND_MODE (DORC + 4)
#define CODE     (CUR_ROUND_MODE + 4)
#define LAST_PC  (CODE + 4)
#define TMP_     (LAST_PC + 4)
#define LL       (TMP_ + 4)
#define ICOUNT   1976
#define CCOUNT   1984
#define NCOUNT   (CCOUNT + 8)
#define VCOUNT   (NCOUNT + 8)
#define NEAREST  (VCOUNT + 8)
#define TRUNC    (NEAREST + 4)
#define CEIL     (TRUNC + 4)
#define FLOOR    (CEIL + 4)

// Type aliases
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef bool     BOOL;

// Page table structure
typedef struct {
    BYTE  *Offset[PAGE_SIZE];
    DWORD Value[PAGE_SIZE];
} dynaPageTableStruct;

extern dynaPageTableStruct dynaPageTable[NUM_PAGES];

// Register map structure
typedef struct {
    BYTE  Reg;
    DWORD BirthDate;
    BYTE  Dirty;
} dynaRegMap;

extern dynaRegMap dynaMapToNative[32];
extern dynaRegMap dynaMapToMips[NATIVE_REGS];
extern dynaRegMap buffer[2048];

// Dynamic compiler globals
extern BYTE  dynaNumFreeRegs;
extern DWORD dynaBirthYear;
extern DWORD dynaNextOpCode;
extern DWORD dynaPreCompileLength;
extern BYTE  *dynaCompileTargetPtr;
extern DWORD *dynaRam;
extern WORD  dynaSrc;
extern DWORD dynaRegPtr;
extern DWORD dynaPCPtr;
extern DWORD dynaCurPage;
extern BYTE  dynaForceFallOut;
extern DWORD dynaLengthTable[PAGE_SIZE];
extern BYTE  *dynaRamPtr;

extern int   dynaNumBPoints;
extern DWORD dynaBPoint[1024];
extern bool  dynaBreakOnKnown;

extern DWORD dasmNumLookups;
extern DWORD *dasmLookupAddy;

extern DWORD InterpHelperAddress;
extern DWORD helperLoadDWordAddress;
extern DWORD helperLoadWordAddress;
extern DWORD helperLoadHalfAddress;
extern DWORD helperLoadByteAddress;
extern DWORD helperStoreDWordAddress;
extern DWORD helperStoreWordAddress;
extern DWORD helperStoreHalfAddress;
extern DWORD helperStoreByteAddress;
extern DWORD helperCheckIntsAddress;
extern DWORD helperLDLAddress;
extern DWORD helperLWLAddress;
extern DWORD helperLDRAddress;
extern DWORD helperLWRAddress;
extern DWORD bugFinderAddress;

extern DWORD SneekyAddress;
extern DWORD dynaPC;
extern DWORD dynaVCount;
extern DWORD dynaICount;
extern BYTE  dynaInBranch;
extern BYTE  *dynaHold;
extern BOOL  dynaHLEAudio;
extern bool  dynaStepMode;
extern bool  dynaBreakSet;

extern volatile WORD NewTask;
extern volatile WORD dynaVTRACE;

extern DWORD dynaCheckIntsAddress;

// Instruction pointer typedef
typedef WORD (*ins_ptr)(BYTE *dst);

extern ins_ptr SpecialInstruction[64];
extern ins_ptr MainInstruction[64];
extern ins_ptr RegimmInstruction[32];
extern ins_ptr Cop0RSInstruction[32];
extern ins_ptr Cop0RTInstruction[32];
extern ins_ptr Cop0Instruction[64];
extern ins_ptr Cop1RSInstruction[32];
extern ins_ptr Cop1RTInstruction[32];
extern ins_ptr Cop1Instruction[64];
extern ins_ptr Cop2RSInstruction[32];

// Function prototypes
#ifdef __cplusplus
extern "C" {
#endif

extern void dynaInit(void);
extern void dynaDestroy(void);
extern BYTE dynaExecute(DWORD Address);
extern void dynaInvalidate(DWORD Start, DWORD Length);
extern BYTE dynaPreCompilePage(DWORD Address);
extern BYTE dynaCompilePage(DWORD Address);
extern void dynaMarkRegDirty(BYTE MipsReg, BYTE IsDirty);
extern BYTE dynaIsUsed(BYTE MipsReg);
extern BYTE dynaGetFreeReg(void);
extern WORD dynaFlushReg(BYTE *cp, BYTE NReg);
extern WORD dynaKillNative(BYTE NReg);
extern WORD dynaTmpFlushReg(BYTE *cp, BYTE NReg);
extern WORD dynaTmpFlushRegPair(BYTE *cp, BYTE MipsReg);
extern void dynaSwitchMap(BYTE a, BYTE b);
extern WORD dynaMakeFreeReg(BYTE *cp, BYTE *NReg);
extern WORD dynaLoadReg(BYTE *cp, BYTE MipsReg);
extern WORD dynaLoadRegPair(BYTE *cp, BYTE MipsReg);
extern WORD dynaLoadRegForceNot(BYTE *cp, BYTE MipsReg, BYTE ExcludedNativeReg);
extern WORD dynaLoadRegForce(BYTE *cp, BYTE MipsReg, BYTE ForcedNativeReg);
extern DWORD dynaGetCompiledAddress(DWORD NewPC);

#ifdef __cplusplus
}
#endif

#endif // DYNA_COMPILER_H
