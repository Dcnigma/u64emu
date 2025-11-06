#include "mmInputDevice.h"
#include <switch.h>
#include <cstring>
#include "ki.h"

const int DEADZONE = 0x10000; // scaled for Switch stick range

mmInputDevice::mmInputDevice() {
    m_NumDevices = 1; // usually just one player
    std::memset(m_NeedInit, 0, sizeof(m_NeedInit));
    std::memset(m_Map, 0, sizeof(m_Map));
}

mmInputDevice::~mmInputDevice() {}

// Stub functions: not needed for Switch
int mmInputDevice::Create() { return 0; }
int mmInputDevice::Open()   { return 0; }
int mmInputDevice::Close()  { return 0; }
uint32_t mmInputDevice::Scan(uint16_t MapValue, uint16_t* Device, uint16_t* Index, char* Text) { return 0xFFFFFFFF; }
uint32_t mmInputDevice::GetKeyboardData(uint16_t ControllerNum, uint16_t* Values) { return 0; }
void mmInputDevice::PickDevices() {}

uint32_t mmInputDevice::MultiScan(uint16_t* Values) {
    uint32_t state = 0;

    // Static pad state
    static PadState pad;
    static bool initialized = false;
    if (!initialized) {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        initialized = true;
    }

    padUpdate(&pad);

    u64 pressed = padGetButtonsDown(&pad);
    u64 held    = padGetButtons(&pad);

    HidAnalogStickState stick = padGetStickPos(&pad, 0);

    // Map buttons to KI controller bits
    // These shifts correspond to original KI mapping in MultiScan
    if (pressed & HidNpadButton_Plus)  state |= (1 << 26); // Start
    if (pressed & HidNpadButton_A)     state |= (1 << 16); // Quick punch
    if (pressed & HidNpadButton_B)     state |= (1 << 17); // Mid punch
    if (pressed & HidNpadButton_X)     state |= (1 << 18); // Fierce punch
    if (pressed & HidNpadButton_Y)     state |= (1 << 19); // Quick kick
    if (held    & HidNpadButton_L)     state |= (1 << 20); // Mid kick
    if (held    & HidNpadButton_R)     state |= (1 << 21); // Fierce kick

    // D-pad / analog stick
    if ((held & HidNpadButton_Up) || stick.y > DEADZONE)    state |= (1 << 22);
    if ((held & HidNpadButton_Down) || stick.y < -DEADZONE) state |= (1 << 23);
    if ((held & HidNpadButton_Left) || stick.x < -DEADZONE) state |= (1 << 24);
    if ((held & HidNpadButton_Right) || stick.x > DEADZONE) state |= (1 << 25);

    // Split state into Values array (like original code)
    Values[0] = ~(state & 0xFFFF);
    Values[1] = ~((state >> 16) & 0xFFFF);
    Values[2] = 0; // Unused

    return state;
}
