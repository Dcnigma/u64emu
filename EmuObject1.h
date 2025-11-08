#pragma once
#include <switch.h>      // must be first
#include <cstdint>
#include <cstdio>

#include "mmSoundBuffer.h"
#include "mmDirectInputDevice.h"
#include "iMain.h"

// Forward declarations
class CPUState;
class EmuState;
class mmDisplay;   // forward declare instead of including the header
class CEmuObject {
public:
    CEmuObject();
    ~CEmuObject();

    bool m_Open;
    bool m_Debug;
    bool m_IsWaveraceSE;

    char m_FileName[256];

    mmDirectInputDevice* m_InputDevice;
    mmDisplay* m_Display;
    mmSoundBuffer* m_Audio;

    EmuState* m;   // pointer to emu state
    CPUState* r;   // pointer to CPU state

    uint32_t m_LastTime;
    uint32_t m_NumVSYNCs;
    uint32_t m_LastInstruction;
    uint32_t m_FirstVSYNCTime;
    bool  m_3DActive;

    uint32_t m_AudioBufferPosition;
    bool m_AudioReady;
    bool m_BadAudio;

    // Public interface
    bool Init();
    void Emulate(const char* filename);
    bool UpdateDisplay();
    void UpdateAudio(uint32_t offset);
    void UpdateInfo();
    void StopEmulation();

    // Input / Timer
    void OnTimer();
    void PollInput();

    // Cleanup / Focus
    bool DestroyWindow();
    void OnDestroy();
    void OnCancel();
    void OnKillFocus(void* pNewWnd);

    // Input handlers (stubbed for Switch)
    void OnKeyDown(uint32_t nChar, uint32_t nRepCnt, uint32_t nFlags);
    void OnKeyUp(uint32_t nChar, uint32_t nRepCnt, uint32_t nFlags);
    void OnChar(uint32_t nChar, uint32_t nRepCnt, uint32_t nFlags);
    void OnSysKeyDown(uint32_t nChar, uint32_t nRepCnt, uint32_t nFlags);

    // Switch doesn’t have a Windows-style mouse, stub as needed
    void OnMouseMove(uint32_t nFlags, void* point);

    bool OnInitDialog();
};
