#include "iRom.h"
#include "iMemory.h"
#include "iCPU.h"
#include "iMain.h"
#include <switch.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

WORD EndianChangeMode = 2;
std::ifstream iRomFileStream;
WORD iRomNumPages;
constexpr size_t ROM_PAGE_SIZE = 0x20000;  // 128 KB
BYTE* iRomPageMap = nullptr;
N64RomStruct* rom = nullptr;

void iRomConstruct() {
    rom = new N64RomStruct{};
    rom->Image = nullptr;
    iRomNumPages = 0;
    iRomPageMap = nullptr;
}

void iRomDestruct() {
    if (iRomFileStream.is_open())
        iRomFileStream.close();
    delete[] rom->Image;
    delete rom;
    delete[] iRomPageMap;
    rom = nullptr;
    iRomPageMap = nullptr;
}

int iRomReadImage(const char* filename) {
    iRomFileStream.open(filename, std::ios::binary);
    if (!iRomFileStream.is_open()) {
        std::cerr << "Failed to open ROM image: " << filename << std::endl;
        return -1;
    }

    iRomFileStream.seekg(0, std::ios::end);
    std::streamsize fileLength = iRomFileStream.tellg();
    iRomFileStream.seekg(0, std::ios::beg);

    // Round up to nearest page
    rom->Length = static_cast<DWORD>(fileLength);
    rom->Length = ((rom->Length / ROM_PAGE_SIZE) + 1) * ROM_PAGE_SIZE;
    rom->PrgCodeLength = rom->Length - 0x1000;

    rom->Image = new BYTE[rom->Length]();
    if (!rom->Image) {
        std::cerr << "Failed to allocate memory for ROM image" << std::endl;
        iRomFileStream.close();
        return -1;
    }

    rom->Header = rom->Image;
    rom->BootCode = rom->Image + 0x40;
    rom->PrgCode = reinterpret_cast<DWORD*>(rom->Image + 0x1000);

    iRomFileStream.read(reinterpret_cast<char*>(rom->Image), fileLength);
    if (!iRomFileStream) {
        std::cerr << "Error reading ROM image" << std::endl;
        iRomFileStream.close();
        return -1;
    }

    if (EndianChangeMode != 2)
        iRomChangeRomEndian(EndianChangeMode, rom->Length);

    iRomFileStream.close();
    return 0;
}

int iRomReadHeader(const char* filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) return -1;

    BYTE header[0x40];
    in.read(reinterpret_cast<char*>(header), sizeof(header));
    if (!in) return -1;

    BYTE b1 = header[0], b2 = header[1], b3 = header[2], b4 = header[3];
    if ((b1 == 0x37 && b2 == 0x80)) EndianChangeMode = 0;
    else if ((b2 == 0x37 && b1 == 0x80)) EndianChangeMode = 1;
    else if ((b3 == 0x37 && b4 == 0x80)) EndianChangeMode = 2;
    else return -2;

    if (EndianChangeMode != 2)
        iRomChangeRomEndian(EndianChangeMode, 0x40);

    rom->Image = new BYTE[0x40];
    memcpy(rom->Image, header, 0x40);
    rom->Header = rom->Image;
    rom->BootCode = rom->Image + 0x40;
    rom->PrgCode = nullptr;

    rom->Info.Validation     = *(WORD*)(rom->Header + (0x00 ^ 0x02));
    rom->Info.Compression    = *(BYTE*)(rom->Header + (0x02 ^ 0x03));
    rom->Info.Unknown1       = *(BYTE*)(rom->Header + (0x03 ^ 0x03));
    rom->Info.Clockrate      = *(DWORD*)(rom->Header + 0x04);
    rom->Info.ProgramCounter = *(DWORD*)(rom->Header + 0x08);
    rom->Info.Release        = *(DWORD*)(rom->Header + 0x0c);
    rom->Info.Crc1           = *(DWORD*)(rom->Header + 0x10);
    rom->Info.Crc2           = *(DWORD*)(rom->Header + 0x14);
    rom->Info.Unknown2       = *(DWORD*)(rom->Header + 0x18);
    rom->Info.Unknown2a      = *(DWORD*)(rom->Header + 0x1c);
    for (int i = 0; i < 20; i++)
        rom->Info.Name[i ^ 0x03] = rom->Header[i + 0x20];

    rom->Info.Unknown3       = *(BYTE*)(rom->Header + (0x34 ^ 0x03));
    rom->Info.Unknown4       = *(BYTE*)(rom->Header + (0x35 ^ 0x03));
    rom->Info.Unknown5       = *(BYTE*)(rom->Header + (0x36 ^ 0x03));
    rom->Info.Unknown6       = *(BYTE*)(rom->Header + (0x37 ^ 0x03));
    rom->Info.Unknown7       = *(BYTE*)(rom->Header + (0x38 ^ 0x03));
    rom->Info.Unknown8       = *(BYTE*)(rom->Header + (0x39 ^ 0x03));
    rom->Info.Unknown9       = *(BYTE*)(rom->Header + (0x3a ^ 0x03));
    rom->Info.ManufacturerId = *(BYTE*)(rom->Header + (0x3b ^ 0x03));
    rom->Info.CartridgeId    = *(WORD*)(rom->Header + (0x3c ^ 0x03));
    rom->Info.CountryCode    = *(BYTE*)(rom->Header + (0x3e ^ 0x03));
    rom->Info.Unknown10      = *(BYTE*)(rom->Header + (0x3f ^ 0x03));
    rom->PrgCodeBaseOrig     = rom->Info.ProgramCounter;
    rom->PrgCodeBase         = rom->PrgCodeBaseOrig & 0x1fffffff;

    return 0;
}

void iRomChangeRomEndian(WORD Mode, DWORD Length) {
    for (DWORD i = 0; i < Length; i += 4) {
        char* p = reinterpret_cast<char*>(rom->Image + i);
        if (Mode == 0) std::swap_ranges(p, p + 4, p); // Swap for mode 0
        else std::swap(p[0], p[3]), std::swap(p[1], p[2]); // Swap for mode 1
    }
}

void iRomChangeRomEndianEx(WORD Mode, DWORD Length, DWORD Offset) {
    for (DWORD i = 0; i < Length; i += 4) {
        char* p = reinterpret_cast<char*>(rom->Image + i + Offset);
        if (Mode == 0) std::swap_ranges(p, p + 4, p);
        else std::swap(p[0], p[3]), std::swap(p[1], p[2]);
    }
}

void iRomSetupPageMap() {
    iRomNumPages = rom->Length / ROM_PAGE_SIZE;
    iRomPageMap = new BYTE[iRomNumPages]();
    for (DWORD p = 0; p < 10; p++)
        iRomReadPage(p);
}

void iRomReadPage(WORD PageNum) {
    // Page caching disabled in modern libnx port
    return;
}

void iRomMapCheck(DWORD Offset, DWORD Length) {
    // Page caching disabled in modern libnx port
    return;
}
