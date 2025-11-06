#pragma once
#include <switch.h>
#include <vector>

class CEmuObject {
public:
    CEmuObject();
    ~CEmuObject();

    bool initialized;
    u32 frameCount;
    static const int width = 16;
    static const int height = 8;
    char screen[height][width + 1];

    std::vector<u8> romData;  // RAII-managed ROM
    u32 pc;

    bool Init();
    void Shutdown();
    bool LoadROM(const char* filename);
    void StepCPU();
    void UpdateDisplay(PadState* pad);
};
