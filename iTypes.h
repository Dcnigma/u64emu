#pragma once
#include <cstdint>

// Basic type definitions
using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;
using QWORD = uint64_t;
using sBYTE = int8_t;
using sWORD = int16_t;
using sDWORD = int32_t;
using sQWORD = int64_t;

// Standard load/store
inline void helperLb(DWORD address, DWORD op0);
inline void helperLbU(DWORD address, DWORD op0);
inline void helperLh(DWORD address, DWORD op0);
inline void helperLhU(DWORD address, DWORD op0);
inline void helperLw(DWORD address, DWORD op0);
inline void helperLwU(DWORD address, DWORD op0);
inline void helperLd(DWORD address, DWORD op0);
inline void helperSb(DWORD address, DWORD op0);
inline void helperSh(DWORD address, DWORD op0);
inline void helperSw(DWORD address, DWORD op0);
inline void helperSd(DWORD address, DWORD op0);

// Unaligned load/store
inline void helperLwl(DWORD address, DWORD op0);
inline void helperLwr(DWORD address, DWORD op0);
inline void helperLdl(DWORD address, DWORD op0);
inline void helperLdr(DWORD address, DWORD op0);
inline void helperSwl(DWORD address, DWORD op0);
inline void helperSwr(DWORD address, DWORD op0);
inline void helperSdl(DWORD address, DWORD op0);
inline void helperSdr(DWORD address, DWORD op0);

// LL/SC helpers
inline void helperSc(DWORD address, DWORD op0);
inline void helperScd(DWORD address, DWORD op0);
inline void helperLl(DWORD address, DWORD op0);
inline void helperLld(DWORD address, DWORD op0);
