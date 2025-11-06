#include "EmuObject1.h"
#include "global.h"
#include "ki.h"
#include "iMemory.h"
#include <stdio.h>
#include <string.h>
#include <switch.h>

CEmuObject::CEmuObject()
{
    m_FileName[0] = '\0';
    m_InputDevice = nullptr;
    m_Debug = false;                   // initialize debug flag
}

CEmuObject::~CEmuObject()
{
}

void CEmuObject::Init()
{
    // Clear SRAM using the global buffer
    memset(SRAM, 0, SRAM_SIZE);
}

void CEmuObject::StopEmulation()
{
    // cleanup if needed
}

bool CEmuObject::UpdateDisplay()
{
    // Placeholder
    return true;
}

void CEmuObject::Emulate(const char* filename)
{
    strncpy(m_FileName, filename, sizeof(m_FileName) - 1);
    m_FileName[sizeof(m_FileName) - 1] = '\0';

    iRomReadImage(m_FileName);
    iMemCopyBootCode();

    printf("Starting emulation for %s\n", m_FileName);

    while (!bQuitSignal)
    {
        UpdateDisplay();
        svcSleepThread(16666667); // ~60Hz
    }

    StopEmulation();
}

void CEmuObject::UpdateAudio(unsigned int size)
{
    // stubbed out for now — you can expand later
    (void)size;
}

uint32_t CEmuObject::ScanInput()
{
    if (!m_InputDevice)
        return 0;

    uint16_t values[4] = {0};
    return m_InputDevice->MultiScan(values);
}
