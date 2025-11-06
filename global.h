#ifndef GLOBAL1_H
#define GLOBAL1_H

#define SRAM_SIZE (2 * 1024 * 1024)   // 2 MB SRAM

#include <stdint.h>
#include <switch.h>
#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

// typedefs...
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef uint64_t QWORD;
// ...other typedefs

extern unsigned char SRAM[SRAM_SIZE]; // single declaration

extern float getTime();  // leave as float if your timing code uses float

#endif // GLOBAL1_H
