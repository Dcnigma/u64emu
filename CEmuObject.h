#pragma once
#include <switch.h>
#include <vector>

class CEmuObject {
public:
    CEmuObject();
    ~CEmuObject();

    // --- CPU / RAM ---
    static const int RAM_SIZE = 16;
    u8 ram[RAM_SIZE];
    bool initialized;
    u32 frameCount;
    std::vector<u8> romData;  // RAII-managed ROM
    u32 pc;

    // --- Screen ---
    static const int width = 16;
    static const int height = 8;
    char screen[height][width + 1];

    // --- Sprite info ---
    static const int spriteWidth = 5;
    static const int spriteHeight = 3;
    static const int spriteFrameCount = 2; // number of frames
    int spriteFrame = 0;                    // current frame

    // VRAM / Tilemap
    static const int VRAM_SIZE = 128;  // Example size, adjust as needed
    u8 vram[VRAM_SIZE];                // VRAM stores tile or pixel data

    // Updated sprite array for multiple frames
    static const int spriteFrames = 1; // start with 1 frame
    char sprite[spriteFrames][spriteHeight][spriteWidth + 1] = {
        {
            " **  ",
            "**** ",
            " **  "
        }
    };

    int spriteX = 0;
    int spriteY = 0;


    // --- Functions ---
    bool Init();
    void Shutdown();
    bool LoadROM(const char* filename);
    void StepCPU(PadState* pad);
    void UpdateDisplay(PadState* pad);
};
