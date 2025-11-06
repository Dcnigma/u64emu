#include <switch.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

bool bQuitSignal = false;

// --- Minimal CKIApp stub ---
class CKIApp {
public:
    void LogMessage(const char* fmt, ...);
    void ErrorMessage(const char* fmt, ...);
};

CKIApp theApp;

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

// --- MAIN ---
int main(int argc, char* argv[])
{
    consoleInit(NULL);
    printf("Hello from CKIApp minimal test!\n");
    printf("Press + to exit.\n");
    fflush(stdout);

    // Simple log test
    theApp.LogMessage("Program started successfully");

#ifdef NXOVERCLOCK
    if (R_SUCCEEDED(pcvInitialize()))
    {
        pcvSetClockRate(PcvModule::PcvModule_Cpu, 1785000000);
        printf("CPU overclocked\n");
        fflush(stdout);
    }
#endif

    // Main loop
    while (appletMainLoop())
    {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
        {
            printf("Exiting...\n");
            fflush(stdout);
            break;
        }

        consoleUpdate(NULL);
    }

#ifdef NXOVERCLOCK
    pcvSetClockRate(PcvModule::PcvModule_Cpu, 1020000000);
    pcvExit();
#endif

    consoleExit(NULL);
    return 0;
}
