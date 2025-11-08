#pragma once
#include <vector>

extern "C" {
#include <switch.h>
}


// Minimal emulator object with simple memory mapping.
// ROM mapped at ROM_BASE..ROM_BASE+romSize-1
// RAM mapped at RAM_BASE..RAM_BASE+RAM_SIZE-1
// VRAM mapped at VRAM_BASE..VRAM_BASE+VRAM_SIZE-1
// PC is a 32-bit address (starts at ROM_BASE)

class CEmuObject {
public:
    CEmuObject();
    ~CEmuObject();

    // sizes
    static const size_t RAM_SIZE   = 64 * 1024; // 64KB RAM (arbitrary, safe)
    static const int width  = 32;   // console width (chars)
    static const int height = 16;   // console height (lines)
    static const size_t VRAM_SIZE  = width * height; // 1 byte per char cell

    // convenient types
    using byte = u8;
    using u32_t = u32;

    // public state (simple)
    bool initialized;
    u32 frameCount;

    // memory containers
    std::vector<byte> romData;
    std::vector<byte> ram;   // size RAM_SIZE
    std::vector<byte> vram;  // size VRAM_SIZE

    // program counter (address space)
    u32 pcAddr;

    // simple console framebuffer (text)
    char screen[height][width + 1]; // last char is '\0'

    // simple sprite (kept for convenience)
    static const int spriteWidth = 5;
    static const int spriteHeight = 3;
    char sprite[spriteHeight][spriteWidth + 1] = {
        " **  ",
        "**** ",
        " **  "
    };
    int spriteX;
    int spriteY;

    // Lifecycle
    bool Init();
    void Shutdown();

    // ROM loading
    bool LoadROM(const char* filename);

    // Memory helpers (address space is internal virtual)
    byte ReadMemory(u32 addr);
    void WriteMemory(u32 addr, byte val);

    // CPU / display
    void StepCPU(PadState* pad);           // one instruction (stub)
    void UpdateDisplay(PadState* pad);     // step CPU multiple times and render

    // constants for mapping
    static constexpr u32 ROM_BASE = 0x00000000u;
    static constexpr u32 RAM_BASE = 0x00100000u;
    static constexpr u32 VRAM_BASE = 0x00200000u;
};
