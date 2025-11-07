#ifndef KI_H_INCLUDED
#define KI_H_INCLUDED

#include <switch.h>
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <string>

// Local emulator headers
#include "global.h"
#include "EmuObject1.h"
#include "mmInputDevice.h"

// Game variants
#define KI1 1
#define KI2 2

extern float getTime();

// ---------------------------------------------------------------------------
// CKIApp
// ---------------------------------------------------------------------------

class CKIApp {
public:
    CKIApp();
    ~CKIApp();

    // Lifecycle
    bool InitInstance(int argc, char* argv[]);
    int ExitInstance();

    // Logging / diagnostics
    void LogMessage(const char* fmt, ...);
    void ErrorMessage(const char* fmt, ...);
    void RSPMessage(const char* fmt, ...);
    void NIMessage(const char* fmt, ...);
    void VectorMessage(const char* fmt, ...);
    void BPMessage(const char* fmt, ...);
    void ABMessage(const char* fmt, ...);

    // Emulator actions
    void LoadGame();
    void SaveGame();

    // Helpers
    FILE* openLogFile(const char* filename, const char* mode);

    // -----------------------------------------------------------------------
    // Members (modernized but mostly 1:1 with original)
    // -----------------------------------------------------------------------

    char m_Path[256]{};
    char m_LastBrowsePath[256]{};
    char m_HomeDirectory[256]{};
    char m_HDImage[256]{};
    char m_ARom1[256]{};
    char m_ARom2[256]{};
    char m_ARom3[256]{};
    char m_ARom4[256]{};
    char m_ARom5[256]{};
    char m_ARom6[256]{};
    char m_ARom7[256]{};
    char m_ARom8[256]{};

    uint16_t m_DIPS = 0;
    uint16_t m_UCode = 0;
    uint32_t m_BootCode = 0;
    uint32_t m_FrameDelay = 0;
    uint32_t m_VTraceInc = 0;

    bool m_Auto = false;
    bool m_Skip = false;
    bool m_ScanLines = false;
    bool m_FakeCOP1 = false;
    bool m_PalShift = false;
    bool m_ShowFPS = false;
    bool m_BypassGFX = false;
    bool m_BypassAFX = false;
    bool m_LogGFX = false;
    bool m_StartLog = false;
    bool m_WireFrame = false;
    bool m_LockOn = false;
    bool m_MFCHack = false;
    bool m_UpdateAudio = false;

    uint16_t m_Indent = 0;
    int m_FullScreen = 0;
    int m_ScreenRes = 0;
    int m_DeviceNum[4]{};
    bool m_FitToWindow = false;
    int m_IgnoreSecondary = 0;
    int m_DisableSound = 0;
    int m_3dfx = 0;

    // Emulator subsystems
    CEmuObject* m_EmuObj = nullptr;
    mmInputDevice* m_InputDevice = nullptr;

private:
    // internal log file handles
    FILE* m_LogFile = nullptr;
    FILE* m_RDPFile = nullptr;
    FILE* m_InsFile = nullptr;
    FILE* m_BPFile = nullptr;

    std::string m_LogPath = "/sdmc/switch/ki/";
};

// ---------------------------------------------------------------------------
// CKIDlg stub (used to satisfy references from legacy code)
// ---------------------------------------------------------------------------
class CKIDlg {
public:
    bool DestroyWindow() { return true; }
    void ShowWindow(int /*nCmd*/) { /* no-op on Switch */ }
};

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
extern bool bQuitSignal;
extern CKIApp theApp;

extern "C" {
    extern uint32_t gRomSet;
    extern uint32_t gAllowHLE;
}

#endif // KI_H_INCLUDED
