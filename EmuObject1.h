#ifndef EMUOBJECT1_H
#define EMUOBJECT1_H

#include "global.h"
#include "mmInputDevice.h"

class CEmuObject {
public:
    CEmuObject();
    ~CEmuObject();

    // Core functions
    void Init();
    void Emulate(const char* filename);
    bool UpdateDisplay();
    void StopEmulation();
    void UpdateAudio(unsigned int size);

    // Optional game functions
    void LoadGame(const char* filename);
    void SaveGame(const char* filename);
    void Reset();

    // Members
    bool m_Debug;                   // debug flag
    char m_FileName[256];           // loaded ROM filename
    mmInputDevice* m_InputDevice;   // input device pointer
    bool m_IsRunning;               // emulation running flag
    DWORD m_CycleCount;             // CPU cycles
    int m_FrameCount;               // frame counter
};

#endif // EMUOBJECT1_H
