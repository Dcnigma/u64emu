#ifndef IROM_H
#define IROM_H

#include <cstdint>

// Standard integer types for clarity
using BYTE  = uint8_t;
using WORD  = uint16_t;
using DWORD = uint32_t;
using QWORD = uint64_t;

// Forward declaration of N64 ROM structure
struct N64RomStruct;

// ROM management
extern N64RomStruct* rom;

// ROM functions
void iRomConstruct();
void iRomDestruct();

int iRomReadImage(const char* filename);
int iRomReadHeader(const char* filename);

void iRomChangeRomEndian(WORD mode, DWORD length);
void iRomChangeRomEndianEx(WORD mode, DWORD length, DWORD offset);

void iRomSetupPageMap();
void iRomReadPage(WORD pageNum);
void iRomMapCheck(DWORD offset, DWORD length);

#endif // IROM_H
