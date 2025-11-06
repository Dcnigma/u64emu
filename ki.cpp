// ki.cpp : Defines the class behaviors for the application.
//
#include "global.h"
#include "ki.h"
#include <switch.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define RELEASE_BUILD

/////////////////////////////////////////////////////////////////////////////
// CKIApp construction

void BootKI1(void);
void BootKI2(void);

bool bQuitSignal;
int gameisrunning = 0;

CKIApp::CKIApp() { /* All init happens in InitInstance */ }

/////////////////////////////////////////////////////////////////////////////
// The one and only CKIApp object
CKIApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CKIApp initialization

bool CKIApp::InitInstance()
{
    gAllowHLE = 0;
    gRomSet = KI2;

}

void CKIApp::LogMessage(char* fmt, ...)
{
#ifndef RELEASE_BUILD
    char ach[1280];
    va_list va;
    va_start(va, fmt);
    vsnprintf(ach, sizeof(ach), fmt, va);
    va_end(va);

    FILE* logFile = fopen("log.dat", "a");
    if (logFile)
    {
        fprintf(logFile, "%s\r\n", ach);
        fclose(logFile);
    }
#endif
}

void CKIApp::ErrorMessage(char* fmt, ...)
{
    char ach[128];
    va_list va;
    va_start(va, fmt);
    vsnprintf(ach, sizeof(ach), fmt, va);
    va_end(va);

    FILE* logFile = fopen("log.dat", "a");
    if (logFile)
    {
        fprintf(logFile, "::Error:: %s\r\n", ach);
        fclose(logFile);
    }
}

void CKIApp::LoadGame() {}
void CKIApp::SaveGame() {}
void CKIApp::RSPMessage(char* fmt, ...) {}
void CKIApp::NIMessage(char* fmt, ...) {}
void CKIApp::VectorMessage(char* fmt, ...) {}
void CKIApp::BPMessage(char* fmt, ...) {}

int CKIApp::ExitInstance() { return 0; }


extern "C"
{
	DWORD gRomSet;
	DWORD gAllowHLE;
}

/////////////////////////////////////////////////////////////////////////////
// Main entry point

int main()
{
    consoleInit(NULL);
    printf("Console initialized\n");
    fflush(stdout);

#ifdef NXOVERCLOCK
    if (R_SUCCEEDED(pcvInitialize()))
    {
        printf("Overclock init\n");
        fflush(stdout);
        pcvSetClockRate(PcvModule::PcvModule_Cpu, 1785000000);
    }
#endif

    printf("About to call BootKI1()\n");
    fflush(stdout);
    BootKI1();

#ifdef NXOVERCLOCK
    printf("Reset clocks\n");
    fflush(stdout);
    pcvSetClockRate(PcvModule::PcvModule_Cpu, 1020000000);
    pcvExit();
#endif

    printf("Exiting main\n");
    fflush(stdout);

    consoleExit(NULL);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Boot functions

void BootKI1(void)
{
    printf("BootKI1: Entered function\n");
    fflush(stdout);

    CEmuObject e;

    theApp.m_UCode = 0;
    theApp.m_BootCode = 0x3f;
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

    gRomSet = KI1;

    printf("BootKI1: Initializing emulator\n");
    fflush(stdout);
    e.Init();
    printf("BootKI1: Emulator initialized\n");
    fflush(stdout);

    printf("BootKI1: Starting emulation with U98-L15D\n");
    fflush(stdout);
    e.Emulate("U98-L15D");
    theApp.m_UCode = 0;

    while (!bQuitSignal)
    {
        e.UpdateDisplay();
        svcSleepThread(16000); // ~16ms delay for 60fps
    }

    e.StopEmulation();
}

void BootKI2(void)
{
    CEmuObject e;

    theApp.m_UCode = 0;
    theApp.m_BootCode = 0x3f;
    theApp.m_DIPS = 3584;
    theApp.m_LockOn = true;
    theApp.m_FrameDelay = (uint32_t)(1000.f / 60.f + 0.3f);
    theApp.m_VTraceInc = 49999980 / 60;

    gAllowHLE = 1;
    bQuitSignal = false;

    strcpy(theApp.m_HDImage, "ki2.img");

    strcpy(theApp.m_ARom1, "ki2_l1.u10");
    strcpy(theApp.m_ARom2, "ki2_l1.u11");
    strcpy(theApp.m_ARom3, "ki2_l1.u12");
    strcpy(theApp.m_ARom4, "ki2_l1.u13");
    strcpy(theApp.m_ARom5, "ki2_l1.u33");
    strcpy(theApp.m_ARom6, "ki2_l1.u34");
    strcpy(theApp.m_ARom7, "ki2_l1.u35");
    strcpy(theApp.m_ARom8, "ki2_l1.u36");

    gRomSet = KI2;

    e.Init();
    e.Emulate("ki2-l11.u98");
    theApp.m_UCode = 0;

    while (!bQuitSignal)
    {
        e.UpdateDisplay();
        svcSleepThread(16000);
    }

    e.StopEmulation();
}
