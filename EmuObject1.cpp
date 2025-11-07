// EmuObject1_nx.cpp : ported for ARM64 libnx (Nintendo Switch)
// Full 1:1 replacement of original EmuObject1.cpp

#include <switch.h>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

#include "ki.h"
#include "EmuObject1.h"
#include "iCPU.h"
#include "iMemory.h"
#include "iRom.h"
#include "hleDSP.h"
#include "adsp2100.h"

// External globals
extern WORD *ataDataBuffer;
extern DWORD lagging;
extern volatile WORD NewTask;
extern DWORD dynaVCount;

extern WORD globalAudioIncRateWhole;
extern WORD globalAudioIncRatePartial;
extern WORD globalAudioIncRateFrac;
extern BOOL dynaHLEAudio;

extern DWORD aiRate;
extern DWORD aiBufLen;
extern DWORD aiUsed;
extern DWORD aiUsedF;
extern DWORD aiUsedW;
extern DWORD aiUsedA;
extern DWORD aiDataUsed;

#define NUM_BUFFERS 64
extern DWORD cheat;

// -------------------------- CEmuObject --------------------------
CEmuObject::CEmuObject()
    : m_Display(nullptr), m_Audio(nullptr), m_Open(false),
      m_LastTime(0), m_LastInstruction(0), m_FirstVSYNCTime(0),
      m_NumVSYNCs(0), m_BadAudio(true), m_3DActive(false),
      m_Debug(false), m_IsWaveraceSE(false)
{
    memset(m_FileName, 0, sizeof(m_FileName));
    m_AudioReady = false;
    m_AudioBufferPosition = 0;
}

CEmuObject::~CEmuObject()
{
    StopEmulation();
}

bool CEmuObject::Init()
{
    m_Display = new mmDisplay();
    GPUInit(640, 480);

    m_DirectSoundDevice = new mmDirectSoundDevice();
    if (m_DirectSoundDevice->Create(0) != 0) {
        delete m_DirectSoundDevice;
        m_DirectSoundDevice = nullptr;
    }

    m_InputDevice = new mmDirectInputDevice();
    m_InputDevice->Create(DISCL_FOREGROUND, nullptr);
    m_InputDevice->Open();

    m_Open = true;
    return true;
}

void CEmuObject::Emulate(const char* filename)
{
    m_LastTime = 0;
    m_LastInstruction = 0;
    m_FirstVSYNCTime = 0;
    m_NumVSYNCs = 0;
    m_Audio = nullptr;
    m_BadAudio = true;

    strncpy(m_FileName, filename, sizeof(m_FileName) - 1);
    m_FileName[sizeof(m_FileName) - 1] = '\0';

    iMainConstruct((char*)m_FileName);
    iRomReadImage((char*)m_FileName);
    iMemCopyBootCode();
    iMainReset();
    iMainStartCPU();

    printf("Emulating %s\n", m_FileName);
}

// -------------------------- Timer / Update loop --------------------------
void CEmuObject::OnTimer()
{
    if (!r) return;

    DWORD curtime = armGetSystemTick();

    // Initialize timing on first call
    if (m_LastTime == 0) {
        m_LastTime = curtime;
        m_LastInstruction = r->ICount;
        m_FirstVSYNCTime = curtime;   // <-- initialize first VSYNC time
        return;
    }

    float deltatime = float(curtime - m_LastTime) / 1000.f;
    if (deltatime == 0.f) return;
    m_LastTime = curtime;

    // Instructions per second
    float ips = float((sQWORD)r->ICount - m_LastInstruction) / deltatime;
    m_LastInstruction = r->ICount;

    // FPS / APS / VSync rate
    float fps = 0.f, aps = 0.f, hz = 0.f;
    if (m_NumVSYNCs > 0) {
        float vsyncDelta = float(curtime - m_FirstVSYNCTime) / 1000.f;
        if (vsyncDelta > 0.f) {
            fps = float(m_Display->m_FrameCount) / vsyncDelta;
            aps = float(aiDataUsed) / vsyncDelta;
            hz  = float(dynaVCount) / vsyncDelta;
        }
        // Reset counters after reporting
        m_NumVSYNCs = 0;
        m_Display->m_FrameCount = 0;
        aiDataUsed = 0;
        dynaVCount = 0;
        m_FirstVSYNCTime = curtime;
    }

    char info[512];
    snprintf(info, sizeof(info),
        "PC:%llX Compare:%08X ICount:%08X NextInt:%08X miReg3:%04X miReg2:%04X | IPS:%.0f FPS:%.1f APS:%.1f VHz:%.1f",
        r->PC, (DWORD)r->CompareCount, (DWORD)r->ICount, r->NextIntCount,
        ((DWORD*)m->miReg)[3], ((DWORD*)m->miReg)[2],
        ips, fps, aps, hz);
    printf("%s\n", info);
}


// -------------------------- Input Handling --------------------------
void CEmuObject::PollInput()
{
    // Scan all controllers
    hidScanInput();

    // Player 1 controller
    u64 kDown = hidKeysDown(CONTROLLER_P1);
    u64 kHeld = hidKeysHeld(CONTROLLER_P1);

    // Reset emulator input state
    inputs[0] = 0;

    // Map buttons to emulator bits
    if (kHeld & KEY_A)      inputs[0] |= 0x0001;
    if (kHeld & KEY_B)      inputs[0] |= 0x0002;
    if (kHeld & KEY_X)      inputs[0] |= 0x0004;
    if (kHeld & KEY_Y)      inputs[0] |= 0x0008;
    if (kHeld & KEY_L)      inputs[0] |= 0x0010;
    if (kHeld & KEY_R)      inputs[0] |= 0x0020;
    if (kHeld & KEY_ZL)     inputs[0] |= 0x0040;
    if (kHeld & KEY_ZR)     inputs[0] |= 0x0080;
    if (kHeld & KEY_PLUS)   inputs[0] |= 0x0100;
    if (kHeld & KEY_MINUS)  inputs[0] |= 0x0200;
    if (kHeld & KEY_DUP)    inputs[0] |= 0x0400;
    if (kHeld & KEY_DDOWN)  inputs[0] |= 0x0800;
    if (kHeld & KEY_DLEFT)  inputs[0] |= 0x1000;
    if (kHeld & KEY_DRIGHT) inputs[0] |= 0x2000;
    if (kHeld & KEY_LSTICK) inputs[0] |= 0x4000;
    if (kHeld & KEY_RSTICK) inputs[0] |= 0x8000;

    // Read analog sticks (left and right)
    HidAnalogStickState leftStick, rightStick;
    hidJoystickRead(&leftStick, CONTROLLER_P1, JOYSTICK_LEFT);
    hidJoystickRead(&rightStick, CONTROLLER_P1, JOYSTICK_RIGHT);

    auto StickToWord = [](float f) -> WORD {
        return WORD((std::clamp(f, -1.0f, 1.0f) + 1.0f) * 0.5f * 65535.0f);
    };

    r->GPR[A0 * 2] = StickToWord(leftStick.x);   // Left stick X
    r->GPR[A1 * 2] = StickToWord(leftStick.y);   // Left stick Y
    r->GPR[A2 * 2] = StickToWord(rightStick.x);  // Right stick X
    r->GPR[A3 * 2] = StickToWord(rightStick.y);  // Right stick Y

    // Gyro/accelerometer (stubbed, can implement later if needed)
    for (int i = 0; i < 6; ++i) r->FPR[i] = 0.f;
}


// -------------------------- Display Update --------------------------
bool CEmuObject::UpdateDisplay()
{
    if (!m_Open) return false;

    DWORD offset = (gRomSet == KI2) ? m->atReg[0x98] : m->atReg[0x80];
    char* source = (char*)SRAM + (offset * 320 * 128 + 0x30000);

    // VSYNC / frame timing
    m_NumVSYNCs++;
    if (m_NumVSYNCs >= 1) {
        // Aim for ~16ms per frame (60Hz)
        static DWORD nextVSync = 0;
        DWORD now = armGetSystemTick();
        if (nextVSync == 0) nextVSync = now + 16;
        if (now < nextVSync) {
            svcSleepThread((nextVSync - now) * 1000); // sleep in microseconds
        }
        nextVSync += 16;
    }

    m_Display->UpdateScreenBuffer(this, source, 0);
    m_Display->m_FrameCount++;

    return true;
}


// -------------------------- Audio Update --------------------------
void CEmuObject::UpdateAudio(DWORD offset)
{
    DWORD length = 0x1e0;
    DWORD rate = 32000;
    WORD bits = 16;

    if (!m_Audio)
    {
        char* src = (char*)&m->dspDMem[dspAutoBase * 2];
        m_Audio = new mmSoundBuffer();
        m_Audio->Create(m_DirectSoundDevice);

        m_BadAudio = !m_Audio->FromMemory(src, length, rate, bits, NUM_BUFFERS, false);
        m_AudioBufferPosition = 0;
        m_AudioReady = false;
    }
    else
    {
        m_AudioBufferPosition = (m_AudioBufferPosition + 1) & (NUM_BUFFERS - 1);
        m_Audio->Update((char*)&m->dspDMem[offset * 2], length, m_AudioBufferPosition);

        if (!m_AudioReady && m_AudioBufferPosition > 6)
        {
            m_Audio->Play(0, 0, true);
            m_AudioReady = true;
        }
    }
}

// -------------------------- Cleanup / Shutdown --------------------------
void CEmuObject::StopEmulation()
{
    if (!m_Open) return;

    if (m_Audio) { m_Audio->Stop(); SafeDelete(m_Audio); }
    if (m_Display) { m_Display->Close(); SafeDelete(m_Display); }
    SafeDelete(m_InputDevice);

    m_Open = false;
}

BOOL CEmuObject::DestroyWindow() { StopEmulation(); return TRUE; }
void CEmuObject::OnDestroy() { StopEmulation(); }
void CEmuObject::OnCancel() { StopEmulation(); }
void CEmuObject::OnKillFocus(void* pNewWnd) {}
void CEmuObject::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {}
void CEmuObject::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) {}
void CEmuObject::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {}
void CEmuObject::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {}
void CEmuObject::OnMouseMove(UINT nFlags, void* point) {}
BOOL CEmuObject::OnInitDialog()
{
    m_LastTime = armGetSystemTick();
    printf("Loading...\n");
    return TRUE;
}
