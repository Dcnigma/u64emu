// iATA_switch.cpp
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <string>
//#include <arpa/inet.h>   // for htons/ntohs if needed
#include "iMain.h"
#include "dynaCompiler.h"
#include "iCPU.h"
#include "iMemory.h"
#include "iRom.h"
#include "iATA.h"

// Portable type definitions
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

std::fstream ataFile;
std::fstream ataMFile;
DWORD ataHeads;       // num of heads per InitParams command
DWORD ataSectors;     // num of sectors per InitParams command

DWORD ataHead;        // current head
DWORD ataSectorCount; // sector count
DWORD ataSectorNum;   // current sector
DWORD ataCylLow;
DWORD ataCylHigh;
WORD *ataCurData = nullptr;
WORD *ataDataBuffer = nullptr;
DWORD ataUsed;
DWORD ataBytesHead;
WORD ataTransferMode = 0;
DWORD ataTargetLen;
DWORD CatchMe = 0;

WORD ataDriveID[6];

#define MIRRORn
#define VERBOSEn

void iATAConstruct()
{
    ataDataBuffer = (WORD *)malloc(512 * 256 * 2); // bytes per sector * max sectors per access
    if (!ataDataBuffer) {
        printf("ATA: Failed to allocate data buffer\n");
        std::abort();
    }
    ataCurData = ataDataBuffer;
}

void iATADestruct()
{
    if (ataDataBuffer) {
        free(ataDataBuffer);
        ataDataBuffer = nullptr;
    }
    if (ataFile.is_open()) {
        ataFile.close();
    }
#ifdef MIRROR
    if (ataMFile.is_open()) {
        ataMFile.close();
    }
#endif
}

bool iATAOpen()
{
    ataFile.open(theApp.m_HDImage, std::ios::in | std::ios::out | std::ios::binary);
    if (!ataFile.is_open()) {
        printf("ATA: Failed to open HD image: %s\n", theApp.m_HDImage.c_str());
        return false;
    }

#ifdef MIRROR
    ataMFile.open("mirror.dat", std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ataMFile.is_open()) {
        printf("ATA: Failed to create mirror file\n");
        return false;
    }

    // Mirror initial data
    ataFile.seekg(0, std::ios::beg);
    ataFile.read(reinterpret_cast<char*>(ataDataBuffer), 1024);
    ataMFile.write(reinterpret_cast<char*>(ataDataBuffer), 1024);
#endif

    m->atReg[0x170] = 0x48; // ATA ready
    m->aiReg[0x170] = 0x48;

    switch (gRomSet)
    {
        case KI2:
            ataDriveID[0] = 0x2020;
            ataDriveID[1] = 0x3030;
            ataDriveID[2] = 0x5354;
            ataDriveID[3] = 0x3931;
            ataDriveID[4] = 0x3530;
            break;
        case KI1:
            ataDriveID[0] = 0x5354;
            ataDriveID[1] = 0x3931;
            ataDriveID[2] = 0x3530;
            ataDriveID[3] = 0x4147;
            ataDriveID[4] = 0x2020;
            break;
    }

    return true;
}

void iATAClose()
{
    if (ataFile.is_open())
        ataFile.close();

#ifdef MIRROR
    if (ataMFile.is_open())
        ataMFile.close();
#endif
}

void iATAUpdate()
{
#ifdef VERBOSE
    printf("ATA DataUsed %u - Command %02X\n", ataUsed, m->atReg[0x138]);
#endif

    ataCurData = ataDataBuffer;
    ataUsed = 0;

    switch (m->atReg[0x138])
    {
        case 0xec:  // IDENTIFY DEVICE
            iATADriveIdentify();
            break;
        case 0x91:  // INIT DRIVE PARAMETERS
            iATAInitParams();
            break;
        case 0x20:  // READ SECTORS
            iATAReadSectors();
            break;
        case 0x30:  // WRITE SECTORS
            iATAWriteSectors();
            break;
        default:
        {
#ifdef VERBOSE
            printf("Unknown ATA command %02X at PC=%08X\n", m->atReg[0x138], r->PC);
#endif
            DWORD tmp = *(DWORD*)&m->rdRam[0x8724c];
            tmp += 4;
            *(DWORD*)&m->rdRam[0x8724c] = tmp;

            *(DWORD*)&m->atReg[0x138] = 0x21;
            r->CPR0[2*CAUSE] = 0x00;
            m->atReg[0x170] = 0;
            memcpy(&ataDataBuffer[0x1a], ataDriveID, sizeof(WORD) * 6);
            ataUsed = 0;
            ataCurData = ataDataBuffer;

            m->aiReg[0x170] = 0x48;
            break;
        }
    }

    iMemToDo = 0;

    // always set DRV RDY
    m->aiReg[0x170] = 0x48;
    *(DWORD*)&m->aiReg[0x138] = 0;

    // Fire IP3 interrupt
    r->CPR0[2*CAUSE] |= 0x800;
}
void iATADriveIdentify()
{
#ifdef VERBOSE
    printf("DriveID\n");
#endif
    if (gRomSet == KI2)
    {
        memcpy(&ataDataBuffer[0x14/2], ataDriveID, sizeof(WORD) * 6);
    }
    else if (gRomSet == KI1)
    {
        memcpy(&ataDataBuffer[0x1a], ataDriveID, sizeof(WORD) * 6);
    }

    ataUsed = 0;
    ataCurData = ataDataBuffer;
}

void iATAInitParams()
{
    ataHeads = m->atReg[0x130] + 1;
    ataSectors = m->atReg[0x110];
    ataBytesHead = ataSectors * 512 * 419;
    CatchMe++;

#ifdef VERBOSE
    printf("ATA InitParams %u heads, %u sectors\n", ataHeads, ataSectors);
#endif
}

void iATAReadSectors()
{
    ataCurData = ataDataBuffer;
    ataUsed = 0;

    ataSectorCount = m->atReg[0x110];
    if (ataSectorCount == 0) ataSectorCount = 256;

    ataSectorNum = m->atReg[0x118];
    ataCylLow = m->atReg[0x120];
    ataCylHigh = m->atReg[0x128];
    ataHead = m->atReg[0x130];

    DWORD addy = (ataSectorNum - 1);
    DWORD cyl = ((ataCylHigh << 8) | ataCylLow);
    addy += (cyl * 40 * 14);
    addy += (ataHead * 40);
    addy *= 512;
    addy += 0xd;

    ataFile.seekg(addy, std::ios::beg);
    ataFile.read(reinterpret_cast<char*>(ataDataBuffer), ataSectorCount * 512);
    ataTransferMode = 0;
    ataTargetLen = ataSectorCount * 512;

#ifdef MIRROR
    ataMFile.write(reinterpret_cast<char*>(ataDataBuffer), ataSectorCount * 512);
#endif

#ifdef VERBOSE
    printf("ATA ReadSectors from %u: Count %u, Head %u, CylHigh %u, CylLow %u, SN %u\n",
        addy, ataSectorCount, ataHead, ataCylHigh, ataCylLow, ataSectorNum);
#endif
}

void iATAWriteSectors()
{
    ataCurData = ataDataBuffer;
    ataUsed = 0;

    ataSectorCount = m->atReg[0x110];
    if (ataSectorCount == 0) ataSectorCount = 256;

    ataSectorNum = m->atReg[0x118];
    ataCylLow = m->atReg[0x120];
    ataCylHigh = m->atReg[0x128];
    ataHead = m->atReg[0x130];

    ataTargetLen = ataSectorCount * 512;
    DWORD addy = (ataSectorNum - 1);
    DWORD cyl = ((ataCylHigh << 8) | ataCylLow);
    addy += (cyl * 40 * 14);
    addy += (ataHead * 40);
    addy *= 512;
    addy += 0xd;

    ataFile.seekp(addy, std::ios::beg);
    // Data write deferred until iATADataRead completes
    ataTransferMode = 1;

#ifdef VERBOSE
    printf("ATA WriteSectors from %u: Count %u, Head %u, CylHigh %u, CylLow %u, SN %u\n",
        addy, ataSectorCount, ataHead, ataCylHigh, ataCylLow, ataSectorNum);
#endif
}

BYTE *iATADataRead()
{
    BYTE *add = reinterpret_cast<BYTE*>(ataCurData);
    ataCurData++;
    ataUsed += 2;

    if (ataUsed >= ataTargetLen)
    {
        if (ataUsed == ataTargetLen)
        {
            if (ataTransferMode)
            {
                ataFile.write(reinterpret_cast<char*>(ataDataBuffer), ataSectorCount * 512);
#ifdef VERBOSE
                printf("ATA WriteData %u\n", ataTargetLen);
#endif
            }
            else
            {
#ifdef VERBOSE
                printf("ATA ReadData %u\n", ataTargetLen);
#endif
            }
        }
        else
        {
            NewTask = DEBUG_BREAK;
        }

        ataCurData = ataDataBuffer;
        ataUsed = 0;
    }

    return add;
}
