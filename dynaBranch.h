#pragma once
/* dynaBranch.h
 * Modernized for libnx / AArch64 (Nintendo Switch)
 * Matches dynaBranch.cpp signatures you provided.
 *
 * - Fixed-width integer types
 * - DWORD is 64-bit (uint64_t) suitable for AArch64 PC/addresses
 * - C++ safe via extern "C"
 * - Optional attribute macros (commented / ready-to-enable)
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Type aliases (match legacy names used in your code) --- */
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint64_t DWORD; /* 64-bit program counters / addresses on AArch64 */

/* --- Optional attribute macros (enable if you want explicit hints) ---
 * Example usage: ATTR_VISIBLE extern void StopMe(void);
 *
 * - ATTR_VISIBLE: make symbol visible from shared objects (default used in libnx)
 * - ATTR_NOINLINE: prevent inlining (useful for functions called from generated code)
 * - ATTR_ALWAYS_INLINE: force inline (only enable for small helper functions)
 *
 * Uncomment the definitions you want to use.
 */

/* #define ATTR_VISIBLE __attribute__((visibility("default"))) */
#define ATTR_VISIBLE
/* #define ATTR_NOINLINE __attribute__((noinline)) */
/* #define ATTR_ALWAYS_INLINE inline __attribute__((always_inline)) */
#define ATTR_NOINLINE

/* --- Function prototypes --- */
/* Keep the original function signatures — implementations in dynaBranch.cpp */
ATTR_VISIBLE void StopMe(void);

ATTR_VISIBLE WORD dynaOpILoop(BYTE *cp, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpEret(BYTE *cp);
ATTR_VISIBLE WORD dynaOpJ(BYTE *cp, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpJr(BYTE *cp, BYTE op0);
ATTR_VISIBLE WORD dynaOpJalr(BYTE *cp, BYTE op0, BYTE op1);
ATTR_VISIBLE WORD dynaOpJal(BYTE *cp, DWORD NewPC);

ATTR_VISIBLE WORD dynaOpBeq(BYTE *cp, BYTE op0, BYTE op1, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBeqILoop(BYTE *cp, BYTE op0, BYTE op1, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBne(BYTE *cp, BYTE op0, BYTE op1, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBneILoop(BYTE *cp, BYTE op0, BYTE op1, DWORD NewPC);

ATTR_VISIBLE WORD dynaOpBlez(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBgtz(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBltz(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBgez(BYTE *cp, BYTE op0, DWORD NewPC);

ATTR_VISIBLE WORD dynaOpBeql(BYTE *cp, BYTE op0, BYTE op1, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBnel(BYTE *cp, BYTE op0, BYTE op1, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBlezl(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBgtzl(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBltzl(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBgezl(BYTE *cp, BYTE op0, DWORD NewPC);

ATTR_VISIBLE WORD dynaOpBltzal(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBgezal(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBltzall(BYTE *cp, BYTE op0, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBgezall(BYTE *cp, BYTE op0, DWORD NewPC);

ATTR_VISIBLE WORD dynaOpBct(BYTE *cp, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBctl(BYTE *cp, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBcf(BYTE *cp, DWORD NewPC);
ATTR_VISIBLE WORD dynaOpBcfl(BYTE *cp, DWORD NewPC);

#ifdef __cplusplus
}
#endif
