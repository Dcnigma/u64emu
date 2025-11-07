#ifndef GLOBAL1_H
#define GLOBAL1_H

#include <switch.h>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cstdint>
#include <cmath>

#define _PI  3.14159265359f
#define _PI2  6.28318530718f
//#define DO_ALL_MATH

#undef RELEASE
#ifdef __cplusplus
#define RELEASE(x) if (x != NULL) {x->Release(); x = NULL;}
#else
#define RELEASE(x) if (x != NULL) {x->lpVtbl->Release(x); x = NULL;}
#endif

// TGA header structure
typedef struct _tgaHeader {
    uint8_t IDLength;
    uint8_t CMapType;
    uint8_t ImgType;
    uint8_t CMapStartLo;
    uint8_t CMapStartHi;
    uint8_t CMapLengthLo;
    uint8_t CMapLengthHi;
    uint8_t CMapDepth;
    uint8_t XOffSetLo;
    uint8_t XOffSetHi;
    uint8_t YOffSetLo;
    uint8_t YOffSetHi;
    uint8_t WidthLo;
    uint8_t WidthHi;
    uint8_t HeightLo;
    uint8_t HeightHi;
    uint8_t PixelDepth;
    uint8_t ImageDescriptor;
} TgaHeader;

// Image types
#define TGA_NULL      0
#define TGA_CMAP      1
#define TGA_TRUE      2
#define TGA_MONO      3
#define TGA_CMAP_RLE  9
#define TGA_TRUE_RLE 10
#define TGA_MONO_RLE 11

#define HACK_1(w) ((float)(w) - 0.5f) / (float)(w)
#define HACK_0(w) 0.5f / (float)(w)

typedef uint64_t QWORD;
typedef int64_t  sQWORD;
typedef int32_t  sDWORD;
typedef uint16_t HWORD;
typedef int16_t  sHWORD;
typedef int8_t   sBYTE;
typedef uint8_t  BIT

#define SafeFree(ptr) if(ptr != NULL) { free(ptr); ptr = NULL; }
#define SafeDelete(ptr) if(ptr != NULL) { delete ptr; ptr = NULL; }
#define SafeRelease(ptr) if(ptr != NULL) { ptr->Release(); }
#define SafeReleaseNoCheck(ptr) if(ptr != NULL) { ptr->Release(); }

#define ToUpper(string) { char *dst = string; char *src = string; while(*src) { *(dst++) = toupper(*(src++)); } }

#endif
