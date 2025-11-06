#include <switch.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <cstdlib>

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

// --- Dummy emulator object with console “screen” ---
class CEmuObject {
public:
    bool initialized;
    u32 frameCount;
    static const int width = 16;
    static const int height = 8;
    char screen[height][width + 1]; // +1 for null terminator

    CEmuObject() : initialized(false), frameCount(0)
    {
        for (int y = 0; y < height; y++)
        {
            screen[y][width] = '\0'; // null terminate each row
        }
    }

    bool Init()
    {
        printf("CEmuObject::Init() called\n");
        fflush(stdout);
        initialized = true;

        // Initialize screen with dots
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                screen[y][x] = '.';

        frameCount = 0;
        return true;
    }

    void Shutdown()
    {
        if (initialized)
        {
            printf("\nCEmuObject::Shutdown() called\n");
            initialized = false;
        }
    }

    void UpdateDisplay()
    {
        if (!initialized) return;

        frameCount++;

        // Example: move a “*” across the screen
        int x = frameCount % width;
        int y = frameCount % height;

        // Clear screen
        for (int row = 0; row < height; row++)
            for (int col = 0; col < width; col++)
                screen[row][col] = '.';

        // Set a moving pixel
        screen[y][x] = '*';

        // Print to console
        printf("\x1b[H"); // ANSI escape: move cursor to top-left
        for (int row = 0; row < height; row++)
            printf("%s\n", screen[row]);
        printf("\nFrame: %u\n", frameCount);
        fflush(stdout);
    }
};



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
    strcpy(theApp.m_ARom1, "U10-L1");
    strcpy(theApp.m_ARom2, "U11-L1");
    strcpy(theApp.m_ARom3, "U12-L1");
    strcpy(theApp.m_ARom4, "U13-L1");
    strcpy(theApp.m_ARom5, "U33-L1");
    strcpy(theApp.m_ARom6, "U34-L1");
    strcpy(theApp.m_ARom7, "U35-L1");
    strcpy(theApp.m_ARom8, "U36-L1");

    gRomSet = 1; // KI1

    printf("BootKI1(): setup done.\n");
    fflush(stdout);

    CEmuObject e;
    if (!e.Init())
    {
        printf("BootKI1(): emulator initialization failed!\n");
        fflush(stdout);
        return;
    }

    printf("BootKI1(): emulator initialized successfully.\n");
    fflush(stdout);

    // --- Minimal emulation loop ---
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

bool emuRunning = true;
while (appletMainLoop() && emuRunning)
{
    padUpdate(&pad);
    u64 kDown = padGetButtonsDown(&pad);

    if (kDown & HidNpadButton_Plus)
    {
        printf("\nBootKI1(): Exiting emulation...\n");
        fflush(stdout);
        emuRunning = false;
    }

    e.UpdateDisplay();           // <-- call the dummy display
    svcSleepThread(16000);       // ~16ms per frame for ~60 FPS

    consoleUpdate(NULL);
}


    // Clean shutdown
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
