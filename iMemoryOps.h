#ifndef IMEMOPS_H
#define IMEMOPS_H

#include <switch.h>

//-------------------- Load/Store Double-Word --------------------
extern void iOpLdl();
extern void iOpLdr();

//-------------------- Byte/Word/Double Loads --------------------
extern void iOpLb();
extern void iOpLh();
extern void iOpLwl();
extern void iOpLw();
extern void iOpLbu();
extern void iOpLhu();
extern void iOpLwr();
extern void iOpLwu();

//-------------------- Byte/Word/Double Stores --------------------
extern void iOpSb();
extern void iOpSh();
extern void iOpSwl();
extern void iOpSw();
extern void iOpSdl();
extern void iOpSdr();
extern void iOpSwr();

//-------------------- LL/SC --------------------
extern void iOpLl();
extern void iOpLld();
extern void iOpSc();
extern void iOpScd();

//-------------------- Floating-point Memory --------------------
extern void iOpLwc1();
extern void iOpLwc2();
extern void iOpLldc1();
extern void iOpLdc1();
extern void iOpLdc2();
extern void iOpSwc1();
extern void iOpSwc2();
extern void iOpSdc1();
extern void iOpSdc2();
extern void iOpSd();

//-------------------- TLB Operations --------------------
extern void iOpTlbr();
extern void iOpTlbwi();
extern void iOpTlbwr();
extern void iOpTlbp();
extern void iOpLui();

#endif // IMEMOPS_H
