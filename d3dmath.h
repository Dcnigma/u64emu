#ifndef __D3DMATH_H__
#define __D3DMATH_H__

#include <math.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Basic types */
typedef float           D3DVALUE;
typedef struct D3DVECTOR {
    float x, y, z, w;   /* w is optional/useful for vector-matrix ops */
} D3DVECTOR, *LPD3DVECTOR;

typedef struct D3DMATRIX {
    /* Row-major layout matching the implementation */
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
} D3DMATRIX, *LPD3DMATRIX;

/* Vector operations */
void D3DVECTORSubtract(LPD3DVECTOR dst, LPD3DVECTOR v1, LPD3DVECTOR v2);
void D3DVECTORAdd(LPD3DVECTOR dst, LPD3DVECTOR v1, LPD3DVECTOR v2);
void D3DVECTORScalarMultiply(LPD3DVECTOR dst, LPD3DVECTOR v1, float Scalar);
LPD3DVECTOR D3DVECTORNormalise(LPD3DVECTOR v);
LPD3DVECTOR D3DVECTORCrossProduct(LPD3DVECTOR dst, LPD3DVECTOR v1, LPD3DVECTOR v2);
D3DVALUE D3DVECTORDotProduct(LPD3DVECTOR v1, LPD3DVECTOR v2);

/* Matrix operations */
LPD3DMATRIX MultiplyD3DMATRIX(LPD3DMATRIX dst, LPD3DMATRIX src1, LPD3DMATRIX src2);
LPD3DMATRIX D3DMATRIXInvert(LPD3DMATRIX dst, LPD3DMATRIX src);
LPD3DMATRIX D3DMATRIXSetRotation(LPD3DMATRIX dst, LPD3DVECTOR dir, LPD3DVECTOR up);

/* Rotation concatenation */
void ConcatenateXRotation(LPD3DMATRIX m, float Radians);
void ConcatenateYRotation(LPD3DMATRIX m, float Radians);
void ConcatenateZRotation(LPD3DMATRIX m, float Radians);

/* Spline */
void spline(LPD3DVECTOR p, float t, LPD3DVECTOR p1, LPD3DVECTOR p2,
            LPD3DVECTOR p3, LPD3DVECTOR p4);

/* Vector-Matrix transforms
   - dst and srcv use D3DVECTOR (x,y,z,w). w should be set by caller (1.0 for positions, 0.0 for normals).
   - srcm is a full 4x4 matrix (row-major as defined above).
*/
void VectorMatrixMultiply(LPD3DVECTOR dst, LPD3DVECTOR srcv, LPD3DMATRIX srcm);
void NormalTransform(LPD3DVECTOR dst, LPD3DVECTOR srcv, LPD3DMATRIX srcm);

#ifdef __cplusplus
}
#endif

#endif /* __D3DMATH_H__ */
