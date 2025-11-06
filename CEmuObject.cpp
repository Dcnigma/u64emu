#include "CEmuObject.h"
#include <cstdio>
#include <cstdlib>

CEmuObject::CEmuObject() : initialized(false), frameCount(0), pc(0) {
    for (int y = 0; y < height; y++)
        screen[y][width] = '\0';
}

CEmuObject::~CEmuObject() {
    Shutdown(); // ensures clean state
}

bool CEmuObject::Init() {
    printf("CEmuObject::Init() called\n");
    fflush(stdout);
    initialized = true;

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            screen[y][x] = '.';

    frameCount = 0;
    pc = 0;

    romData.clear(); // ensure no old data
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
    return true;
}

void CEmuObject::StepCPU() {
    if (pc >= romData.size()) return;
    u8 opcode = romData[pc++];
    screen[0][0] = (opcode % 2 == 0) ? '*' : '.';
}

void CEmuObject::UpdateDisplay(PadState* pad) {
    if (!initialized) return;

    for (int i = 0; i < 10; i++)
        StepCPU();

    frameCount++;

    padUpdate(pad);
    u64 kDown = padGetButtonsDown(pad);

    if (kDown & HidNpadButton_Up)    screen[1][1] = '^';
    if (kDown & HidNpadButton_Down)  screen[1][2] = 'v';

    printf("\x1b[2J\x1b[H");
    for (int y = 0; y < height; y++)
        printf("%s\n", screen[y]);
    printf("Frame: %u PC: %u\n", frameCount, pc);
    fflush(stdout);

    consoleUpdate(NULL);
}
