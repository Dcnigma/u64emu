#include "d3dmath.h"
#include <string.h>
#include <arm_neon.h>
#include <math.h>

// Vector operations
void D3DVECTORSubtract(LPD3DVECTOR dst, LPD3DVECTOR v1, LPD3DVECTOR v2) {
    dst->x = v1->x - v2->x;
    dst->y = v1->y - v2->y;
    dst->z = v1->z - v2->z;
}

void D3DVECTORAdd(LPD3DVECTOR dst, LPD3DVECTOR v1, LPD3DVECTOR v2) {
    dst->x = v1->x + v2->x;
    dst->y = v1->y + v2->y;
    dst->z = v1->z + v2->z;
}

void D3DVECTORScalarMultiply(LPD3DVECTOR dst, LPD3DVECTOR v1, float Scalar) {
    dst->x = v1->x * Scalar;
    dst->y = v1->y * Scalar;
    dst->z = v1->z * Scalar;
}

LPD3DVECTOR D3DVECTORNormalise(LPD3DVECTOR v) {
    float mag = sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);
    if(mag == 0.f) { v->y = 1.f; return v; }
    float inv = 1.f/mag;
    v->x *= inv; v->y *= inv; v->z *= inv;
    return v;
}

LPD3DVECTOR D3DVECTORCrossProduct(LPD3DVECTOR dst, LPD3DVECTOR v1, LPD3DVECTOR v2) {
    dst->x = v1->y*v2->z - v1->z*v2->y;
    dst->y = v1->z*v2->x - v1->x*v2->z;
    dst->z = v1->x*v2->y - v1->y*v2->x;
    return dst;
}

D3DVALUE D3DVECTORDotProduct(LPD3DVECTOR v1, LPD3DVECTOR v2) {
    return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}

// Matrix multiplication
LPD3DMATRIX MultiplyD3DMATRIX(LPD3DMATRIX dst, LPD3DMATRIX src1, LPD3DMATRIX src2) {
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++) {
            float sum = 0.f;
            for(int k=0;k<4;k++)
                sum += ((float*)src1)[i*4+k] * ((float*)src2)[k*4+j];
            ((float*)dst)[i*4+j] = sum;
        }
    return dst;
}

// Matrix inversion (transpose rotation+copy translation)
LPD3DMATRIX D3DMATRIXInvert(LPD3DMATRIX dst, LPD3DMATRIX src) {
    dst->_11 = src->_11; dst->_12 = src->_21; dst->_13 = src->_31; dst->_14 = src->_14;
    dst->_21 = src->_12; dst->_22 = src->_22; dst->_23 = src->_32; dst->_24 = src->_24;
    dst->_31 = src->_13; dst->_32 = src->_23; dst->_33 = src->_33; dst->_34 = src->_34;
    dst->_41 = src->_14; dst->_42 = src->_24; dst->_43 = src->_34; dst->_44 = src->_44;
    return dst;
}

// Set rotation matrix from direction/up
LPD3DMATRIX D3DMATRIXSetRotation(LPD3DMATRIX dst, LPD3DVECTOR dir, LPD3DVECTOR up) {
    D3DVECTOR d = *dir; D3DVECTOR u = *up, r;
    D3DVECTORNormalise(&d);
    float t = u.x*d.x + u.y*d.y + u.z*d.z;
    u.x -= d.x*t; u.y -= d.y*t; u.z -= d.z*t;
    D3DVECTORNormalise(&u);
    D3DVECTORCrossProduct(&r, &u, &d);

    dst->_11 = r.x; dst->_12 = r.y; dst->_13 = r.z;
    dst->_21 = u.x; dst->_22 = u.y; dst->_23 = u.z;
    dst->_31 = d.x; dst->_32 = d.y; dst->_33 = d.z;
    return dst;
}

// Spline interpolation
void spline(LPD3DVECTOR p, float t, LPD3DVECTOR p1, LPD3DVECTOR p2, LPD3DVECTOR p3, LPD3DVECTOR p4) {
    float t2 = t*t, t3 = t2*t;
    float m1 = (-1.f*t3 + 2.f*t2 - t)/2.f;
    float m2 = (3.f*t3 - 5.f*t2 + 2.f)/2.f;
    float m3 = (-3.f*t3 + 4.f*t2 + t)/2.f;
    float m4 = (t3 - t2)/2.f;
    p->x = p1->x*m1 + p2->x*m2 + p3->x*m3 + p4->x*m4;
    p->y = p1->y*m1 + p2->y*m2 + p3->y*m3 + p4->y*m4;
    p->z = p1->z*m1 + p2->z*m2 + p3->z*m3 + p4->z*m4;
}

// Rotation concatenation helpers
void ConcatenateXRotation(LPD3DMATRIX m, float Radians) {
    float Sin = sinf(Radians), Cos = cosf(Radians);
    float a[4][4]; memcpy(a,m,sizeof(D3DMATRIX));
    m->_12 = a[0][1]*Cos + a[0][2]*Sin;
    m->_22 = a[1][1]*Cos + a[1][2]*Sin;
    m->_32 = a[2][1]*Cos + a[2][2]*Sin;
    m->_42 = a[3][1]*Cos + a[3][2]*Sin;
    m->_13 = a[0][1]*-Sin + a[0][2]*Cos;
    m->_23 = a[1][1]*-Sin + a[1][2]*Cos;
    m->_33 = a[2][1]*-Sin + a[2][2]*Cos;
    m->_43 = a[3][1]*-Sin + a[3][2]*Cos;
}

void ConcatenateYRotation(LPD3DMATRIX m, float Radians) {
    float Sin = sinf(Radians), Cos = cosf(Radians);
    float a[4][4]; memcpy(a,m,sizeof(D3DMATRIX));
    m->_11 = a[0][0]*Cos + a[0][2]*-Sin;
    m->_21 = a[1][0]*Cos + a[1][2]*-Sin;
    m->_31 = a[2][0]*Cos + a[2][2]*-Sin;
    m->_41 = a[3][0]*Cos + a[3][2]*-Sin;
    m->_13 = a[0][0]*Sin + a[0][2]*Cos;
    m->_23 = a[1][0]*Sin + a[1][2]*Cos;
    m->_33 = a[2][0]*Sin + a[2][2]*Cos;
    m->_43 = a[3][0]*Sin + a[3][2]*Cos;
}

void ConcatenateZRotation(LPD3DMATRIX m, float Radians) {
    float Sin = sinf(Radians), Cos = cosf(Radians);
    float a[4][4]; memcpy(a,m,sizeof(D3DMATRIX));
    m->_11 = a[0][0]*Cos + a[0][1]*Sin;
    m->_21 = a[1][0]*Cos + a[1][1]*Sin;
    m->_31 = a[2][0]*Cos + a[2][1]*Sin;
    m->_41 = a[3][0]*Cos + a[3][1]*Sin;
    m->_12 = a[0][0]*-Sin + a[0][1]*Cos;
    m->_22 = a[1][0]*-Sin + a[1][1]*Cos;
    m->_32 = a[2][0]*-Sin + a[2][1]*Cos;
    m->_42 = a[3][0]*-Sin + a[3][1]*Cos;
}

// Vector-Matrix multiplication (NEON optimized)
void VectorMatrixMultiply(LPD3DVECTOR dst, LPD3DVECTOR srcv, LPD3DMATRIX srcm) {
#ifdef __ARM_NEON
    float32x4_t v = vld1q_f32(&srcv->x);
    float32x4_t r1 = vld1q_f32(&srcm->_11);
    float32x4_t r2 = vld1q_f32(&srcm->_21);
    float32x4_t r3 = vld1q_f32(&srcm->_31);
    float32x4_t r4 = vld1q_f32(&srcm->_41);
    float32x4_t res = vmulq_n_f32(r1, vgetq_lane_f32(v,0));
    res = vmlaq_n_f32(res, r2, vgetq_lane_f32(v,1));
    res = vmlaq_n_f32(res, r3, vgetq_lane_f32(v,2));
    res = vmlaq_n_f32(res, r4, vgetq_lane_f32(v,3));
    vst1q_f32(&dst->x, res);
#else
    dst->x = srcv->x*srcm->_11 + srcv->y*srcm->_21 + srcv->z*srcm->_31 + srcv->w*srcm->_41;
    dst->y = srcv->x*srcm->_12 + srcv->y*srcm->_22 + srcv->z*srcm->_32 + srcv->w*srcm->_42;
    dst->z = srcv->x*srcm->_13 + srcv->y*srcm->_23 + srcv->z*srcm->_33 + srcv->w*srcm->_43;
    dst->w = srcv->x*srcm->_14 + srcv->y*srcm->_24 + srcv->z*srcm->_34 + srcv->w*srcm->_44;
#endif
}

// Normal transform (NEON optimized)
void NormalTransform(LPD3DVECTOR dst, LPD3DVECTOR srcv, LPD3DMATRIX srcm) {
#ifdef __ARM_NEON
    float32x4_t v = vld1q_f32(&srcv->x);
    float32x4_t r1 = vld1q_f32(&srcm->_11);
    float32x4_t r2 = vld1q_f32(&srcm->_21);
    float32x4_t r3 = vld1q_f32(&srcm->_31);
    float32x4_t res = vmulq_n_f32(r1, vgetq_lane_f32(v,0));
    res = vmlaq_n_f32(res, r2, vgetq_lane_f32(v,1));
    res = vmlaq_n_f32(res, r3, vgetq_lane_f32(v,2));
    vst1q_f32(&dst->x, res);
    dst->w = 0.f;
#else
    dst->x = srcv->x*srcm->_11 + srcv->y*srcm->_21 + srcv->z*srcm->_31;
    dst->y = srcv->x*srcm->_12 + srcv->y*srcm->_22 + srcv->z*srcm->_32;
    dst->z = srcv->x*srcm->_13 + srcv->y*srcm->_23 + srcv->z*srcm->_33;
    dst->w = 0.f;
#endif
}
