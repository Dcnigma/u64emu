#ifndef GLOBAL1_H
#define GLOBAL1_H

#define SRAM_SIZE (2 * 1024 * 1024)   // 2 MB SRAM

#include <stdint.h>
#include <switch.h>
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

// Type definitions
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef uint64_t QWORD;

// Memory macros
#define SafeFree(ptr) if(ptr!=NULL) { free(ptr); ptr=NULL; }
#define SafeDelete(ptr) if(ptr!=NULL) { delete ptr; ptr=NULL; }
#define SafeRelease(ptr) if(ptr!=NULL) { ptr->Release();  }

// Global SRAM declaration
extern unsigned char SRAM[SRAM_SIZE];

// Timing function
extern float getTime();

#endif // GLOBAL1_H
