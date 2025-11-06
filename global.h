#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
#include <switch.h>
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

// ----- Configuration -----
#define SRAM_SIZE (2 * 1024 * 1024)   // 2 MB SRAM

// ----- Type definitions -----
typedef void* HANDLE;
typedef long HRESULT;

typedef unsigned char BYTE;
typedef signed char sBYTE;
typedef unsigned short WORD;
typedef signed short sWORD;
typedef unsigned int DWORD;
typedef signed int sDWORD;
typedef int64_t sQWORD;
typedef uint64_t QWORD;

typedef unsigned char UINT8;
typedef signed char INT8;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned int UINT32;
typedef signed int INT32;
typedef int64_t INT64;
typedef uint64_t UINT64;

typedef unsigned char BIT;
typedef int BOOL;
typedef unsigned short HWORD;
typedef signed short sHWORD;

#define _PI  3.14159265359f
#define _PI2 6.28318530718f

// ----- Safety macros -----
#define SafeFree(ptr) if(ptr!=NULL) { free(ptr); ptr=NULL; }
#define SafeDelete(ptr) if(ptr!=NULL) { delete ptr; ptr=NULL; }
#define SafeRelease(ptr) if(ptr!=NULL) { ptr->Release(); }

// ----- Global SRAM -----
extern unsigned char SRAM[SRAM_SIZE];

// ----- Timing -----
extern float getTime();

#endif // GLOBAL_H
