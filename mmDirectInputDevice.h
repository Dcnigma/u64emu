#ifndef SC_INPUT
#define SC_INPUT

#include <switch.h>
#include <cstring>
#include "global.h"

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
    uint32_t GetData(int ControllerNum);          // Returns button state as bitfield
    uint32_t MultiScan(uint16_t *Values);        // Like DirectInput MultiScan
    DWORD Scan(uint16_t MapValue, uint16_t *Device, uint16_t *Index, char *Text);

    MacroTask LastTask;                           // Last macro task triggered

private:
    void UpdateMacro(const NpadState &state);    // Detect macros from buttons

    HidControllerID controllers[8];              // Up to 8 controllers
    NpadState npadStates[8];                      // State of each controller
    bool NeedInit[8];                             // First-time init flags
    uint32_t OldJoyData[8][32];                   // Previous DI-like state
    uint64_t lastMacroTime;                       // Macro debounce timer
};

#endif
