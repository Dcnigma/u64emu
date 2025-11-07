#include "CEmuObject.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

CEmuObject::CEmuObject()
    : initialized(false), frameCount(0), pcAddr(ROM_BASE),
      spriteX(0), spriteY(0)
{
    // allocate RAM/VRAM vectors
    ram.resize(RAM_SIZE);
    vram.resize(VRAM_SIZE);

    // init screen strings
    for (int y = 0; y < height; ++y) screen[y][width] = '\0';
    // clear memory
    std::fill(ram.begin(), ram.end(), 0);
    std::fill(vram.begin(), vram.end(), 0);
}

CEmuObject::~CEmuObject() {
    Shutdown();
}

bool CEmuObject::Init() {
    printf("CEmuObject::Init() called\n");
    fflush(stdout);
    initialized = true;
    frameCount = 0;
    pcAddr = ROM_BASE;
    spriteX = 0;
    spriteY = 0;

    // clear framebuffer
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            screen[y][x] = '.';

    // clear memory containers (keep capacity)
    std::fill(ram.begin(), ram.end(), 0);
    std::fill(vram.begin(), vram.end(), 0);

    // romData is kept until LoadROM called
    return true;
}

void CEmuObject::Shutdown() {
    if (initialized) {
        printf("\nCEmuObject::Shutdown() called\n");
        initialized = false;
    }
    romData.clear();
    std::fill(ram.begin(), ram.end(), 0);
    std::fill(vram.begin(), vram.end(), 0);
}

// Load ROM bytes into romData and set PC to ROM_BASE
bool CEmuObject::LoadROM(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("LoadROM: Could not open %s\n", filename);
        fflush(stdout);
        return false;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size == 0) {
        fclose(f);
        printf("LoadROM: File empty\n");
        fflush(stdout);
        return false;
    }

    romData.resize(size);
    size_t read = fread(romData.data(), 1, size, f);
    fclose(f);
    if (read != size) {
        printf("LoadROM: short read: %zu/%zu\n", read, size);
        fflush(stdout);
        return false;
    }

    // PC starts at ROM_BASE (address 0 within ROM)
    pcAddr = ROM_BASE;
    printf("LoadROM: Loaded %zu bytes\n", size);
    fflush(stdout);

    // optional boot seed: copy first bytes into a small RAM area
    size_t seed = std::min<size_t>(4, romData.size());
    for (size_t i = 0; i < seed; ++i) ram[i] = romData[i];

    return true;
}

// ReadMemory: maps virtual address to our buffers
CEmuObject::byte CEmuObject::ReadMemory(u32 addr) {
    // ROM region
    if (addr >= ROM_BASE && addr < ROM_BASE + (u32)romData.size()) {
        u32 index = addr - ROM_BASE;
        return romData[index];
    }
    // RAM region
    if (addr >= RAM_BASE && addr < RAM_BASE + (u32)RAM_SIZE) {
        u32 index = addr - RAM_BASE;
        return ram[index];
    }
    // VRAM region
    if (addr >= VRAM_BASE && addr < VRAM_BASE + (u32)VRAM_SIZE) {
        u32 index = addr - VRAM_BASE;
        return vram[index];
    }
    // unmapped -> return 0
    return 0;
}

void CEmuObject::WriteMemory(u32 addr, byte val) {
    if (addr >= RAM_BASE && addr < RAM_BASE + (u32)RAM_SIZE) {
        u32 index = addr - RAM_BASE;
        ram[index] = val;
        return;
    }
    if (addr >= VRAM_BASE && addr < VRAM_BASE + (u32)VRAM_SIZE) {
        u32 index = addr - VRAM_BASE;
        vram[index] = val;
        return;
    }
    // ROM writes ignored (read-only). Silently ignore unmapped writes.
}

// Single-step CPU: fetch from memory at PC and execute a tiny subset of ops
// PC is an address in our virtual address space.
void CEmuObject::StepCPU(PadState* pad) {
    if (romData.empty()) return;

    // make sure pcAddr points into ROM
    if (pcAddr < ROM_BASE || pcAddr >= ROM_BASE + (u32)romData.size()) {
        // invalid PC: wrap to start of ROM
        pcAddr = ROM_BASE;
        return;
    }

    byte opcode = ReadMemory(pcAddr);
    pcAddr++; // advance to next byte (simple 8-bit instruction stream)

    switch (opcode) {
        case 0x00: // NOP
            break;

        case 0x01: // increment RAM[0]
        {
            u32 a = RAM_BASE + 0;
            byte v = ReadMemory(a);
            WriteMemory(a, (byte)(v + 1));
        }
        break;

        // simple sprite movement instructions (relative)
        case 0x02: if (spriteX < width - spriteWidth) spriteX++; break;
        case 0x03: if (spriteX > 0) spriteX--; break;
        case 0x04: if (spriteY > 0) spriteY--; break;
        case 0x05: if (spriteY < height - spriteHeight) spriteY++; break;

        // read pad and move sprite accordingly
        case 0x10:
            padUpdate(pad);
            if (padGetButtonsDown(pad) & HidNpadButton_Up)    spriteY--;
            break;
        case 0x11:
            padUpdate(pad);
            if (padGetButtonsDown(pad) & HidNpadButton_Down)  spriteY++;
            break;
        case 0x12:
            padUpdate(pad);
            if (padGetButtonsDown(pad) & HidNpadButton_Left)  spriteX--;
            break;
        case 0x13:
            padUpdate(pad);
            if (padGetButtonsDown(pad) & HidNpadButton_Right) spriteX++;
            break;

        // write next N bytes from ROM into VRAM (example)
        case 0x50: {
            // guard: if pcAddr at end, do nothing
            u32 remainingRom = (ROM_BASE + (u32)romData.size()) - pcAddr;
            u32 toCopy = std::min<u32>( (u32)16, remainingRom );
            for (u32 i = 0; i < toCopy && i < VRAM_SIZE; ++i) {
                byte b = ReadMemory(pcAddr++);
                WriteMemory(VRAM_BASE + i, b);
            }
        } break;

        // loop/jump using small ram value
        case 0x30: {
            byte j = ReadMemory(RAM_BASE + 0);
            u32 romsz = (u32)romData.size();
            if (romsz > 0) pcAddr = ROM_BASE + (j % romsz);
        } break;

        // halt: set pcAddr to end-of-rom so StepCPU returns no-ops
        case 0xFF:
            pcAddr = ROM_BASE + (u32)romData.size();
            break;

        default:
            // unknown opcode: ignore
            break;
    }

    // clamp sprite coords
    if (spriteX < 0) spriteX = 0;
    if (spriteX > width - spriteWidth) spriteX = width - spriteWidth;
    if (spriteY < 0) spriteY = 0;
    if (spriteY > height - spriteHeight) spriteY = height - spriteHeight;
}

// Step the CPU multiple times and render a simple text framebuffer from VRAM + sprite
void CEmuObject::UpdateDisplay(PadState* pad) {
    if (!initialized) return;

    // run a few instructions per frame
    for (int i = 0; i < 8; ++i) StepCPU(pad);

    frameCount++;

    // build screen from VRAM (each vram byte -> pixel on/off)
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            screen[y][x] = (vram[y * width + x] % 2 == 0) ? '.' : '#';

    // overlay sprite
    for (int sy = 0; sy < spriteHeight; ++sy) {
        for (int sx = 0; sx < spriteWidth; ++sx) {
            char c = sprite[sy][sx];
            if (c != ' ')
                screen[spriteY + sy][spriteX + sx] = c;
        }
    }

    // optional RAM effect
    screen[0][0] = (ram[0] % 2 == 0) ? '*' : '.';

    // print the text framebuffer
    printf("\x1b[2J\x1b[H"); // clear + home
    for (int y = 0; y < height; ++y)
        printf("%s\n", screen[y]);
    printf("Frame: %u PC: 0x%08x ROMsize:%zu\n", frameCount, (unsigned)pcAddr, romData.size());
    fflush(stdout);

    consoleUpdate(NULL);
}
