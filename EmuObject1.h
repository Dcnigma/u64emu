#pragma once
#include <cstdint>
#include <cstdio>
#include "mmDisplay.h"
#include "mmSoundBuffer.h"
#include "mmDirectInputDevice.h"
#include "iMain.h"

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int BOOL;

class CPUState;   // forward declare
class EmuState;   // forward declare

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
    mmDirectSoundDevice* m_DirectSoundDevice;
    mmSoundBuffer* m_Audio;

    EmuState* m;  // pointer to emu state
    CPUState* r;  // pointer to CPU state

    DWORD m_LastTime;
    DWORD m_NumVSYNCs;
    DWORD m_LastInstruction;
    DWORD m_FirstVSYNCTime;
    bool  m_3DActive;

    DWORD m_AudioBufferPosition;
    bool m_AudioReady;
    bool m_BadAudio;

    // Public interface
    bool Init();
    void Emulate(const char* filename);
    bool UpdateDisplay();
    void UpdateAudio(DWORD offset);
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

    // Input handlers (stubbed)
    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnMouseMove(UINT nFlags, void* point);

    bool OnInitDialog();
};
