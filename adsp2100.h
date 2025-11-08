#ifndef _ADSP2100_H
#define _ADSP2100_H

#include <cstdint>
#include "adsp2100_core.h"

// ---------------------- Config ----------------------
#define SUPPORT_2101_EXTENSIONS 1

// ---------------------- Memory access (platform layer) ----------------------
extern uint8_t  dspReadByte(uint32_t addr);
extern uint16_t dspReadWord(uint32_t addr);
extern uint32_t dspReadDWord(uint32_t addr);
extern void     dspWriteByte(uint32_t addr, uint8_t val);
extern void     dspWriteWord(uint32_t addr, uint16_t val);
extern void     dspWriteDWord(uint32_t addr, uint32_t val);

// ---------------------- Core API ----------------------
extern int adsp2100_icount;

extern void adsp2100_reset(void* param);
extern int  adsp2100_execute(int cycles);
extern void adsp2100_set_irq_line(int irqline, int state);

#if SUPPORT_2101_EXTENSIONS
typedef int32_t (*RX_CALLBACK)(int port);
typedef void (*TX_CALLBACK)(int port, int32_t data);

extern void adsp2100_set_rx_callback(RX_CALLBACK cb);
extern void adsp2100_set_tx_callback(TX_CALLBACK cb);
#endif

// ---------------------- Inline helpers ----------------------
inline int32_t alu_get_x(unsigned n) { return adsp2100.core.ax0.s; } // adjust per register mapping
inline int32_t alu_get_y(unsigned n) { return adsp2100.core.ay0.s; }
inline int32_t alu_get_af() { return adsp2100.core.af.s; }
inline void alu_set_af(int32_t val) { adsp2100.core.af.s = val; }

#endif
