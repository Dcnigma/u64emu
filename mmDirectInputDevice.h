#ifndef SC_INPUT
#define SC_INPUT

#include <switch.h>
#include <switch/services/hid.h>
#include <cstdint>
#include <cstring>
#include "global.h"

// Replace Windows types if needed
using DWORD = uint32_t;
using WORD  = uint16_t;
using BOOL  = bool;

#define KEYBOARD 0
#define JOYSTICK 1

enum MacroTask {
    NONE = 0,
    SAVE_GAME,
    LOAD_GAME,
    TOGGLE_FPS,
    TOGGLE_LOGJAL
};

class mmDirectInputDevice
{
public:
    mmDirectInputDevice();
    ~mmDirectInputDevice();

    // Main data
    uint32_t GetData(int ControllerNum);            // Returns button state as bitfield
    uint32_t MultiScan(uint16_t *Values);          // Like DirectInput MultiScan
    uint32_t Scan(uint16_t MapValue, uint16_t *Device, uint16_t *Index, char *Text);

    MacroTask LastTask;                             // Last macro task triggered

private:
    void UpdateMacro(const PadState &state);        // Detect macros from buttons

//    ControllerId controllers[8];   // Up to 8 controllers
    PadState npadStates[8];        // State of each controller
    bool NeedInit[8];              // First-time init flags
    uint32_t OldJoyData[8][32];    // Previous DI-like state
    uint64_t lastMacroTime;        // Macro debounce timer
};

#endif
