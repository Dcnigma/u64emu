#include "CEmuObject.h"
#include <cstdio>
#include <cstdlib>
#include <cstring> // for memset

CEmuObject::CEmuObject()
    : initialized(false), frameCount(0), pc(0), spriteX(0), spriteY(0)
{
    // Initialize screen
    for (int y = 0; y < height; y++)
        screen[y][width] = '\0';

    // Clear RAM
    std::memset(ram, 0, RAM_SIZE);
}

CEmuObject::~CEmuObject() {
    Shutdown(); // ensures clean state
}

bool CEmuObject::Init() {
    printf("CEmuObject::Init() called\n");
    fflush(stdout);
    initialized = true;

    // Clear screen
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            screen[y][x] = '.';

    frameCount = 0;
    pc = 0;

    std::memset(ram, 0, RAM_SIZE);
    romData.clear(); // ensure no old data

    // Optional: setup a boot region in RAM if CPU expects it
    // for (int i = 0; i < 4; i++) ram[i] = 0x00;

    return true;
}

void CEmuObject::Shutdown() {
    if (initialized) {
        printf("\nCEmuObject::Shutdown() called\n");
        initialized = false;
    }
    romData.clear(); // automatic memory cleanup
}

bool CEmuObject::LoadROM(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("LoadROM: Could not open %s\n", filename);
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    romData.resize(size); // vector allocates automatically
    fread(romData.data(), 1, size, f);
    fclose(f);

    pc = 0;
    printf("LoadROM: Loaded %zu bytes\n", size);

    // Optional: copy first 4 bytes to RAM as boot/initialization
    if (size >= 4) {
        for (int i = 0; i < 4; i++)
            ram[i] = romData[i];
    }

    return true;
}

void CEmuObject::StepCPU(PadState* pad)
{
    if (!romData.size() || pc >= romData.size()) return;

    // Fetch opcode
    u8 opcode = romData[pc++];

    switch(opcode) {
        case 0x00: break; // NOP
        case 0x01: ram[0]++; break;
        case 0x02: if (spriteX < width - spriteWidth) spriteX++; break;
        case 0x03: if (spriteX > 0) spriteX--; break;
        case 0x04: if (spriteY > 0) spriteY--; break;
        case 0x05: if (spriteY < height - spriteHeight) spriteY++; break;

        case 0x10: padUpdate(pad); if (padGetButtonsDown(pad) & HidNpadButton_Up) spriteY--; break;
        case 0x11: padUpdate(pad); if (padGetButtonsDown(pad) & HidNpadButton_Down) spriteY++; break;
        case 0x12: padUpdate(pad); if (padGetButtonsDown(pad) & HidNpadButton_Left) spriteX--; break;
        case 0x13: padUpdate(pad); if (padGetButtonsDown(pad) & HidNpadButton_Right) spriteX++; break;

        // Load next 16 bytes of ROM into VRAM
        case 0x50: {
            int len = (pc + 16 < romData.size()) ? 16 : romData.size() - pc;
            for (int i = 0; i < len && i < VRAM_SIZE; i++)
                vram[i] = romData[pc++];
        } break;

        default: break;
    }

    // Clamp sprite inside bounds
    if (spriteX < 0) spriteX = 0;
    if (spriteX > width - spriteWidth) spriteX = width - spriteWidth;
    if (spriteY < 0) spriteY = 0;
    if (spriteY > height - spriteHeight) spriteY = height - spriteHeight;
}




void CEmuObject::UpdateDisplay(PadState* pad) {
    if (!initialized) return;

    // Step CPU multiple times per frame
    for (int i = 0; i < 10; i++)
        StepCPU(pad);

    frameCount++;

    // --- Clear screen ---
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            screen[y][x] = '.';

    // --- Draw VRAM ---
    for (int i = 0; i < width * height && i < VRAM_SIZE; i++) {
        int x = i % width;
        int y = i / width;
        screen[y][x] = (vram[i] % 2 == 0) ? '.' : '#';
    }

    // --- Draw sprite on top ---
    for (int y = 0; y < spriteHeight; y++) {
        for (int x = 0; x < spriteWidth; x++) {
            char c = sprite[spriteFrame][y][x];
            if (c != ' ')
                screen[spriteY + y][spriteX + x] = c;
        }
    }

    // Optional: small effect from RAM[0]
    screen[0][0] = (ram[0] % 2 == 0) ? '*' : '.';

    // --- Console output ---
    printf("\x1b[2J\x1b[H");
    for (int y = 0; y < height; y++)
        printf("%s\n", screen[y]);
    printf("Frame: %u PC: %u\n", frameCount, pc);
    fflush(stdout);

    consoleUpdate(NULL);
}
