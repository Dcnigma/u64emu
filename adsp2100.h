#ifndef _ADSP2100_H
#define _ADSP2100_H

#include <stdint.h>

/* ---------------------- Standard Integer Types ---------------------- */
typedef uint8_t  BYTE;
typedef int8_t   INT8;
typedef uint16_t UINT16;
typedef int16_t  INT16;
typedef uint32_t UINT32;
typedef int32_t  INT32;
typedef uint64_t UINT64;
typedef int64_t  INT64;
typedef uint32_t DWORD;

#define INLINE inline

/* ---------------------- Config ---------------------- */
#define HAS_ADSP2105 1
#define SUPPORT_2101_EXTENSIONS 1

/* ---------------------- External Dependencies ---------------------- */
#include "ki.h"

extern BYTE *DSPPMem;
extern BYTE *DSPDMem;

extern BYTE  dspReadByte(DWORD A);
extern UINT16 dspReadWord(DWORD A);
extern DWORD dspReadDWord(DWORD A);
extern void  dspWriteByte(DWORD A, BYTE V);
extern void  dspWriteWord(DWORD A, UINT16 V);
extern void  dspWriteDWord(DWORD A, DWORD V);

/* ---------------------- Memory Map ---------------------- */
#define ADSP2100_DATA_OFFSET 0x00000
#define ADSP2100_PGM_OFFSET  0x10000
#define ADSP2100_SIZE        0x20000

#define ADSP_DATA_ADDR_RANGE(start, end) (ADSP2100_DATA_OFFSET + ((start) << 1)), (ADSP2100_DATA_OFFSET + ((end) << 1) + 1)
#define ADSP_PGM_ADDR_RANGE(start, end)  (ADSP2100_PGM_OFFSET + ((start) << 2)), (ADSP2100_PGM_OFFSET + ((end) << 2) + 3)

/* ---------------------- Register Enumeration ---------------------- */
enum {
    ADSP2100_PC=1,
    ADSP2100_AX0, ADSP2100_AX1, ADSP2100_AY0, ADSP2100_AY1, ADSP2100_AR, ADSP2100_AF,
    ADSP2100_MX0, ADSP2100_MX1, ADSP2100_MY0, ADSP2100_MY1, ADSP2100_MR0, ADSP2100_MR1, ADSP2100_MR2, ADSP2100_MF,
    ADSP2100_SI, ADSP2100_SE, ADSP2100_SB, ADSP2100_SR0, ADSP2100_SR1,
    ADSP2100_I0, ADSP2100_I1, ADSP2100_I2, ADSP2100_I3, ADSP2100_I4, ADSP2100_I5, ADSP2100_I6, ADSP2100_I7,
    ADSP2100_L0, ADSP2100_L1, ADSP2100_L2, ADSP2100_L3, ADSP2100_L4, ADSP2100_L5, ADSP2100_L6, ADSP2100_L7,
    ADSP2100_M0, ADSP2100_M1, ADSP2100_M2, ADSP2100_M3, ADSP2100_M4, ADSP2100_M5, ADSP2100_M6, ADSP2100_M7,
    ADSP2100_PX, ADSP2100_CNTR, ADSP2100_ASTAT, ADSP2100_SSTAT, ADSP2100_MSTAT,
    ADSP2100_PCSP, ADSP2100_CNTRSP, ADSP2100_STATSP, ADSP2100_LOOPSP,
    ADSP2100_IMASK, ADSP2100_ICNTL, ADSP2100_IRQSTATE0, ADSP2100_IRQSTATE1, ADSP2100_IRQSTATE2, ADSP2100_IRQSTATE3,
    ADSP2100_FLAGIN, ADSP2100_FLAGOUT
#if SUPPORT_2101_EXTENSIONS
    , ADSP2100_FL0, ADSP2100_FL1, ADSP2100_FL2
#endif
};

/* ---------------------- Interrupt Constants ---------------------- */
#define ADSP2100_INT_NONE -1
#define ADSP2100_IRQ0      0
#define ADSP2100_IRQ1      1
#define ADSP2100_IRQ2      2
#define ADSP2100_IRQ3      3

/* ---------------------- Global Variables ---------------------- */
extern int adsp2100_icount;

/* ---------------------- Core API Functions ---------------------- */
extern void adsp2100_reset(void *param);
extern void adsp2100_exit(void);
extern int  adsp2100_execute(int cycles);
extern unsigned adsp2100_get_context(void *dst);
extern void     adsp2100_set_context(void *src);
extern unsigned adsp2100_get_pc(void);
extern void     adsp2100_set_pc(unsigned val);
extern unsigned adsp2100_get_sp(void);
extern void     adsp2100_set_sp(unsigned val);
extern unsigned adsp2100_get_reg(int regnum);
extern void     adsp2100_set_reg(int regnum, unsigned val);
extern void     adsp2100_set_nmi_line(int state);
extern void     adsp2100_set_irq_line(int irqline, int state);
extern void     adsp2100_set_irq_callback(int (*callback)(int irqline));
extern const char *adsp2100_info(void *context, int regnum);
extern unsigned adsp2100_dasm(char *buffer, unsigned pc);
extern unsigned adsp2100_get_mstat(void);
extern void adsp2100_set_imask(unsigned val);

#if SUPPORT_2101_EXTENSIONS
typedef INT32 (*RX_CALLBACK)(int port);
typedef void  (*TX_CALLBACK)(int port, INT32 data);
extern void adsp2105_set_rx_callback(RX_CALLBACK cb);
extern void adsp2105_set_tx_callback(TX_CALLBACK cb);
#endif

/* ---------------------- Memory Access Macros ---------------------- */
#define ADSP2100_RDMEM(A)      ((unsigned)dspReadByte(A))
#define ADSP2100_RDMEM_WORD(A) ((unsigned)dspReadWord(A))
#define ADSP2100_WRMEM(A,V)    (dspWriteByte(A,V))
#define ADSP2100_WRMEM_WORD(A,V) (dspWriteWord(A,V))

#ifdef MAME_DEBUG
extern unsigned DasmADSP2100(char *buffer, unsigned pc);
#endif

/* ---------------------- ADSP2105 compatibility ---------------------- */
#if HAS_ADSP2105
#define adsp2105_icount adsp2100_icount

#define ADSP2105_INT_NONE -1
#define ADSP2105_IRQ0     0
#define ADSP2105_IRQ1     1
#define ADSP2105_IRQ2     2

extern void adsp2105_reset(void *param);
extern void adsp2105_exit(void);
extern int  adsp2105_execute(int cycles);
extern unsigned adsp2105_get_context(void *dst);
extern void     adsp2105_set_context(void *src);
extern unsigned adsp2105_get_pc(void);
extern void     adsp2105_set_pc(unsigned val);
extern unsigned adsp2105_get_sp(void);
extern void     adsp2105_set_sp(unsigned val);
extern unsigned adsp2105_get_reg(int regnum);
extern void     adsp2105_set_reg(int regnum, unsigned val);
extern void     adsp2105_set_nmi_line(int state);
extern void     adsp2105_set_irq_line(int irqline, int state);
extern void     adsp2105_set_irq_callback(int (*callback)(int irqline));
extern const char *adsp2105_info(void *context, int regnum);
extern unsigned adsp2105_dasm(char *buffer, unsigned pc);
#endif /* HAS_ADSP2105 */

#endif /* _ADSP2100_H */
