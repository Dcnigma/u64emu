#pragma once
#include <switch.h>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <cmath>

#define KEYDOWN(name,key) (name[key] & 0x80)

constexpr int DEADZONE = 0x10000;
constexpr float DEADZONE_NORMALIZED = static_cast<float>(DEADZONE) / 32768.0f;

uint32_t KIMap[32] = {
    0x00000001,0x00000002,0x00000004,0x00000008,
    0x00000010,0x00000020,0x00000040,0x00000080,
    0x00000100,0x00000200,0x00000400,0x00000800,
    0x00001000,0x00002000,0x00004000,0x00008000,
    0x00010000,0x00020000,0x00040000,0x00080000,
    0x00100000,0x00200000,0x00400000,0x00800000,
    0x01000000,0x02000000,0x04000000,0x08000000,
    0x10000000,0x20000000,0x40000000,0x80000000
};

enum MacroTask {
    NONE = 0,
    SAVE_GAME,
    LOAD_GAME,
    TOGGLE_FPS,
    TOGGLE_LOGJAL
};

class mmDirectInputDevice {
public:
    mmDirectInputDevice();
    ~mmDirectInputDevice();

    void Create();
    uint32_t GetData(int ControllerNum);
    uint32_t Scan(uint16_t MapValue, uint16_t *Device, uint16_t *Index, char *Text);
    uint32_t MultiScan(uint16_t *Values);

    MacroTask LastTask;

private:
    HidControllerID controllers[8];
    NpadState npadStates[8];
    bool NeedInit[8];
    uint32_t OldJoyData[8][32];
    uint64_t lastMacroTime;

    void UpdateMacro(const NpadState &state);
    BYTE ScaleAxis(float val);
};

mmDirectInputDevice::mmDirectInputDevice() {
    for(int i=0;i<8;i++) {
        controllers[i] = CONTROLLER_P1_AUTO;
        memset(&npadStates[i],0,sizeof(NpadState));
        NeedInit[i] = true;
        memset(OldJoyData[i],0,sizeof(OldJoyData[i]));
    }
    LastTask = NONE;
    lastMacroTime = 0;
}

mmDirectInputDevice::~mmDirectInputDevice() { }

void mmDirectInputDevice::Create() {
    npadConfigureInput(1, HidNpadStyleSet_NpadStandard);
    npadInitialize(nullptr);
}

BYTE mmDirectInputDevice::ScaleAxis(float val) {
    if(fabs(val) < DEADZONE_NORMALIZED) val = 0.0f;
    val = (val + 1.0f) / 2.0f;
    if(val < 0.0f) val = 0.0f;
    if(val > 1.0f) val = 1.0f;
    return static_cast<BYTE>(val * 255.0f);
}

uint32_t mmDirectInputDevice::GetData(int ControllerNum) {
    if(ControllerNum<0 || ControllerNum>=8) return 0;

    NpadState state;
    npadGetState(&state, controllers[ControllerNum]);

    if(NeedInit[ControllerNum]) {
        memset(OldJoyData[ControllerNum], 0, sizeof(OldJoyData[ControllerNum]));
        NeedInit[ControllerNum]=false;
    }

    uint32_t result = 0;

    // Map buttons
    if(state.buttons & HidNpadButton_A)      result |= KIMap[0];
    if(state.buttons & HidNpadButton_B)      result |= KIMap[1];
    if(state.buttons & HidNpadButton_X)      result |= KIMap[2];
    if(state.buttons & HidNpadButton_Y)      result |= KIMap[3];
    if(state.buttons & HidNpadButton_L)      result |= KIMap[4];
    if(state.buttons & HidNpadButton_R)      result |= KIMap[5];
    if(state.buttons & HidNpadButton_ZL)     result |= KIMap[6];
    if(state.buttons & HidNpadButton_ZR)     result |= KIMap[7];
    if(state.buttons & HidNpadButton_StickL) result |= KIMap[8];
    if(state.buttons & HidNpadButton_StickR) result |= KIMap[9];
    if(state.buttons & HidNpadButton_Plus)   result |= KIMap[10];
    if(state.buttons & HidNpadButton_Minus)  result |= KIMap[11];

    if(state.buttons & HidNpadButton_Up)     result |= KIMap[12];
    if(state.buttons & HidNpadButton_Down)   result |= KIMap[13];
    if(state.buttons & HidNpadButton_Left)   result |= KIMap[14];
    if(state.buttons & HidNpadButton_Right)  result |= KIMap[15];

    // Analog sticks virtual buttons
    if(state.stickL.x > DEADZONE_NORMALIZED)  result |= KIMap[16];
    if(state.stickL.x < -DEADZONE_NORMALIZED) result |= KIMap[17];
    if(state.stickL.y > DEADZONE_NORMALIZED)  result |= KIMap[18];
    if(state.stickL.y < -DEADZONE_NORMALIZED) result |= KIMap[19];

    if(state.stickR.x > DEADZONE_NORMALIZED)  result |= KIMap[20];
    if(state.stickR.x < -DEADZONE_NORMALIZED) result |= KIMap[21];
    if(state.stickR.y > DEADZONE_NORMALIZED)  result |= KIMap[22];
    if(state.stickR.y < -DEADZONE_NORMALIZED) result |= KIMap[23];

    // Pack analog sticks
    OldJoyData[ControllerNum][0] = (ScaleAxis(state.stickL.y) << 8) | ScaleAxis(state.stickL.x);
    OldJoyData[ControllerNum][1] = (ScaleAxis(state.stickR.y) << 8) | ScaleAxis(state.stickR.x);

    UpdateMacro(state);

    return result;
}

void mmDirectInputDevice::UpdateMacro(const NpadState &state) {
    uint64_t now = armGetSystemTick()/192;
    if(now - lastMacroTime < 250) return;

    if((state.buttons & HidNpadButton_L) &&
       (state.buttons & HidNpadButton_R) &&
       (state.buttons & HidNpadButton_X)) {
        LastTask = SAVE_GAME;
        lastMacroTime = now;
    } else if((state.buttons & HidNpadButton_L) &&
              (state.buttons & HidNpadButton_R) &&
              (state.buttons & HidNpadButton_Y)) {
        LastTask = LOAD_GAME;
        lastMacroTime = now;
    } else if(state.buttons & HidNpadButton_Plus) {
        LastTask = TOGGLE_FPS;
        lastMacroTime = now;
    } else if(state.buttons & HidNpadButton_Minus) {
        LastTask = TOGGLE_LOGJAL;
        lastMacroTime = now;
    } else {
        LastTask = NONE;
    }
}

DWORD mmDirectInputDevice::Scan(WORD MapValue, WORD *Device, WORD *Index, char *Text)
{
    NpadState state;
    npadGetState(&state, controllers[0]);

    if(state.buttons & HidNpadButton_A)    { *Device = 0; *Index = 0; strcpy(Text,"Button A"); return KIMap[0]; }
    if(state.buttons & HidNpadButton_B)    { *Device = 0; *Index = 1; strcpy(Text,"Button B"); return KIMap[1]; }
    if(state.buttons & HidNpadButton_X)    { *Device = 0; *Index = 2; strcpy(Text,"Button X"); return KIMap[2]; }
    if(state.buttons & HidNpadButton_Y)    { *Device = 0; *Index = 3; strcpy(Text,"Button Y"); return KIMap[3]; }
    if(state.buttons & HidNpadButton_L)    { *Device = 0; *Index = 4; strcpy(Text,"Button L"); return KIMap[4]; }
    if(state.buttons & HidNpadButton_R)    { *Device = 0; *Index = 5; strcpy(Text,"Button R"); return KIMap[5]; }
    if(state.buttons & HidNpadButton_ZL)   { *Device = 0; *Index = 6; strcpy(Text,"Button ZL"); return KIMap[6]; }
    if(state.buttons & HidNpadButton_ZR)   { *Device = 0; *Index = 7; strcpy(Text,"Button ZR"); return KIMap[7]; }
    if(state.buttons & HidNpadButton_Plus) { *Device = 0; *Index = 10; strcpy(Text,"Button Plus"); return KIMap[10]; }
    if(state.buttons & HidNpadButton_Minus){ *Device = 0; *Index = 11; strcpy(Text,"Button Minus"); return KIMap[11]; }

    return 0xffffffff;
}

uint32_t mmDirectInputDevice::MultiScan(WORD *Values)
{
    NpadState state;
    npadGetState(&state, controllers[0]);

    uint32_t result = GetData(0);

    Values[0] = OldJoyData[0][0];
    Values[1] = OldJoyData[0][1];
    Values[2] = 0;
    Values[3] = 0;

    return result;
}
