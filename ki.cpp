#include <switch.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <cstdlib>
#include "CEmuObject.h"

// --- Globals ---
bool bQuitSignal = false;
u32 gRomSet = 0;
u32 gAllowHLE = 0;

// --- Minimal CKIApp stub ---
class CKIApp {
public:
    u32 m_UCode;
    u32 m_BootCode;
    u32 m_DIPS;
    bool m_LockOn;
    u32 m_FrameDelay;
    u32 m_VTraceInc;
    char m_HDImage[64];
    char m_ARom1[32], m_ARom2[32], m_ARom3[32], m_ARom4[32];
    char m_ARom5[32], m_ARom6[32], m_ARom7[32], m_ARom8[32];

    void LogMessage(const char* fmt, ...);
    void ErrorMessage(const char* fmt, ...);
};

CKIApp theApp;

// --- Logging ---
void CKIApp::LogMessage(const char* fmt, ...)
{
    FILE* f = fopen("log.txt", "a");
    if (!f) return;
    va_list va;
    va_start(va, fmt);
    vfprintf(f, fmt, va);
    fprintf(f, "\n");
    va_end(va);
    fclose(f);
}

void CKIApp::ErrorMessage(const char* fmt, ...)
{
    FILE* f = fopen("log.txt", "a");
    if (!f) return;
    va_list va;
    va_start(va, fmt);
    fprintf(f, "ERROR: ");
    vfprintf(f, fmt, va);
    fprintf(f, "\n");
    va_end(va);
    fclose(f);
}



// --- BootKI1 (running version) ---
void BootKI1(void)
{
    printf("BootKI1(): setting up emulator structures...\n");
    fflush(stdout);

    theApp.m_UCode = 0;
    theApp.m_BootCode = 0x3F;
    theApp.m_DIPS = 3584;
    theApp.m_LockOn = true;
    theApp.m_FrameDelay = (uint32_t)(1000.f / 60.f + 0.3f);
    theApp.m_VTraceInc = 49999980 / 60;

    gAllowHLE = 1;
    bQuitSignal = false;

    strcpy(theApp.m_HDImage, "ki.img");
    gRomSet = 1; // KI1

    printf("BootKI1(): setup done.\n");
    fflush(stdout);

    // --- Create and initialize emulator ---
    CEmuObject e;
    if (!e.Init()) return;
    if (!e.LoadROM(theApp.m_HDImage))
    {
        printf("BootKI1(): Failed to load ROM.\n");
        return;
    }

    printf("BootKI1(): emulator initialized successfully.\n");
    fflush(stdout);

    // --- Setup controller input ---
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    // --- Emulation loop ---
    bool emuRunning = true;
    while (appletMainLoop() && emuRunning)
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
        {
            printf("BootKI1(): Exiting emulation...\n");
            fflush(stdout);
            emuRunning = false;
            break;
        }

        // Update emulator: CPU step + display
        e.UpdateDisplay(&pad);

        // 60 FPS delay
        svcSleepThread(16000);
    }

    // --- Clean shutdown ---
    e.Shutdown();
    printf("BootKI1(): exited cleanly.\n");
    fflush(stdout);
}


// --- BootKI2 stub ---
void BootKI2(void)
{
    printf("BootKI2(): Not implemented yet.\n");
    fflush(stdout);
}

// --- Main ---
int main(int argc, char* argv[])
{
    consoleInit(NULL);
    printf("Killer Instinct Switch Loader\n");
    printf("------------------------------\n");
    printf("A = Boot KI (ki.img)\n");
    printf("B = Boot KI2 (ki2.img) [not yet implemented]\n");
    printf("+ = Exit\n");
    fflush(stdout);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    bool running = true;

    while (appletMainLoop() && running)
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
        {
            printf("Exiting...\n");
            fflush(stdout);
            running = false;
        }
        else if (kDown & HidNpadButton_A)
        {
            printf("Calling BootKI1()...\n");
            fflush(stdout);
            BootKI1();
        }
        else if (kDown & HidNpadButton_B)
        {
            printf("Calling BootKI2()...\n");
            fflush(stdout);
            BootKI2();
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
