#ifndef EMUOBJECT1_H
#define EMUOBJECT1_H

#include "global.h"
#include "mmInputDevice.h"
#include <cstdio>
#include <cstring>

// Forward declaration
class mmInputDevice;

class CEmuObject
{
public:
    CEmuObject();
    ~CEmuObject();
    // … existing declarations …
    void UpdateAudio(unsigned int size);
    bool m_Debug;
    void Init();
    void StopEmulation();
    bool UpdateDisplay();
    void Emulate(const char* filename);
    uint32_t ScanInput();

    char m_FileName[256];
    mmInputDevice* m_InputDevice;
};


#endif // EMUOBJECT1_H
