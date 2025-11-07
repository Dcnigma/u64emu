#pragma once
#include <stdint.h>
#include <stddef.h>
#include <switch.h>

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;

// ===================== Memory Operations =====================
// ARM64-native dynamic memory access operations

extern "C" {

// Load operations
WORD dynaOpSmartLb(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLbU(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLh(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLhU(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLw(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLwU(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLd(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);

// Store operations
WORD dynaOpSmartSb(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartSh(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartSw(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartSd(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);

// FPU Load/Store
WORD dynaOpSmartSwc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartSdc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLwc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLdc1(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);

// Second set of memory ops (Bank2)
WORD dynaOpSmartLb2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLbU2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLh2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLhU2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLw2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLwU2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartLd2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);

WORD dynaOpSmartSb2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartSh2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartSw2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);
WORD dynaOpSmartSd2(BYTE *cp, BYTE op0, BYTE op1, DWORD Imm, BYTE Page);

// Memory map builders
void dynaBuildReadMap();
void dynaBuildWriteMap();

} // extern "C"

// ===================== Notes =====================
// - ARM64 backend uses pure interpreter or dynamic recompiler logic.
// - All functions maintain original signatures.
// - BYTE/WORD/DWORD types are mapped to uint8_t/uint16_t/uint32_t for libnx.
// - To implement the functions, use inline ARM64 assembly or NEON intrinsics for optimal speed.
// - Compatible with aarch64-none-elf-g++ for Switch homebrew builds.
