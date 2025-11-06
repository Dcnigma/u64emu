#ifndef KI_H_INCLUDED
#define KI_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include "global.h"
#include "EmuObject1.h"
#include "mmInputDevice.h"

#define KI1 1
#define KI2 2

extern float getTime();
/////////////////////////////////////////////////////////////////////////////
// CKIApp

class CKIApp
{
public:
    CKIApp();

    char m_Path[256];

    uint16_t m_DIPS;
    uint16_t m_UCode;
    uint32_t m_BootCode;
    uint32_t m_FrameDelay;
    uint32_t m_VTraceInc;

    bool m_Auto;
    bool m_Skip;
    bool m_ScanLines;
    bool m_FakeCOP1;
    bool m_PalShift;
    bool m_ShowFPS;
    bool m_BypassGFX;
    bool m_BypassAFX;
    bool m_LogGFX;
    bool m_StartLog;
    bool m_WireFrame;
    bool m_LockOn;
    uint16_t m_Indent;
    int m_FullScreen;
    int m_ScreenRes;
    int m_DeviceNum[4];
    bool m_FitToWindow;
    int m_IgnoreSecondary;
    int m_DisableSound;
    int m_3dfx;
    char m_LastBrowsePath[256];
    char m_HomeDirectory[256];
    char m_HDImage[256];
    char m_ARom1[256];
    char m_ARom2[256];
    char m_ARom3[256];
    char m_ARom4[256];
    char m_ARom5[256];
    char m_ARom6[256];
    char m_ARom7[256];
    char m_ARom8[256];

    bool m_MFCHack;
    bool m_UpdateAudio;

    void LogMessage(char* fmt, ...);
    void ErrorMessage(char* fmt, ...);
    void RSPMessage(char* fmt, ...);
    void NIMessage(char* fmt, ...);
    void VectorMessage(char* fmt, ...);
    void BPMessage(char* fmt, ...);
    void ABMessage(char* fmt, ...);
    void LoadGame();
    void SaveGame();

    CEmuObject* m_EmuObj;
    mmInputDevice* m_InputDevice;

    int ExitInstance();
    bool InitInstance();
};

/////////////////////////////////////////////////////////////////////////////
// CKIDlg stub for Switch (no Win32)
class CKIDlg {
public:
    bool DestroyWindow() { return true; }
    void ShowWindow(int nCmd) { /* nothing */ }
};
extern bool bQuitSignal;
extern CKIApp theApp;

extern "C" {
	extern DWORD gRomSet;
	extern DWORD gAllowHLE;
}

#endif // KI_H_INCLUDED
