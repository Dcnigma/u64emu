#ifndef MMINPUTDEVICE_H
#define MMINPUTDEVICE_H

#include <cstdint>

#define KEYBOARD 0
#define JOYSTICK 1

class mmInputDevice
{
public:
    mmInputDevice();
    ~mmInputDevice();

    bool m_NeedInit[8];
    uint32_t m_Map[22];
    uint16_t m_NumDevices;
    char m_DeviceName[8][64];
    uint16_t m_DeviceNumber[8];
    uint16_t m_DeviceType[8];
    uint16_t m_Mapping[8][16];

    // Functions
    int Create();
    int Open();
    int Close();
    uint32_t GetData(uint16_t ControllerNum);
    uint32_t GetKeyboardData(uint16_t ControllerNum, uint16_t* Values);
    void PickDevices();
    void KeyToText(int code, char* str);
    uint32_t Scan(uint16_t MapValue, uint16_t* Device, uint16_t* Index, char* Text);
    uint32_t MultiScan(uint16_t* Values);
};

#endif // MMINPUTDEVICE_H
