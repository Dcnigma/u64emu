#pragma once
#include <cstdint>
#include <cmath>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Floating Point Operations ====================
void iOpFAdd();     // FPR[FD] = FPR[FS] + FPR[FT]
void iOpFSub();     // FPR[FD] = FPR[FS] - FPR[FT]
void iOpFMul();     // FPR[FD] = FPR[FS] * FPR[FT]
void iOpFDiv();     // FPR[FD] = FPR[FS] / FPR[FT]
void iOpFSqrt();    // FPR[FD] = sqrt(FPR[FS])
void iOpFAbs();     // FPR[FD] = fabs(FPR[FS])
void iOpFNeg();     // FPR[FD] = -FPR[FS]
void iOpFMov();     // FPR[FD] = FPR[FS]

// ==================== Floating Point Rounding ====================
void iOpFRoundl();  // Round to long int
void iOpFTruncl();  // Truncate to long int
void iOpFCeill();   // Ceil to long int
void iOpFFloorl();  // Floor to long int
void iOpFRoundw();  // Round to int
void iOpFTruncw();  // Truncate to int
void iOpFCeilw();   // Ceil to int
void iOpFFloorw();  // Floor to int

// ==================== Conversion Operations ====================
void iOpFc();       // Convert integer to float
void iOpFCvts();    // Convert to single-precision
void iOpFCvtd();    // Convert to double-precision
void iOpFCvtw();    // Convert to word/int32
void iOpFCvtl();    // Convert to long/int64

#ifdef __cplusplus
}
#endif
