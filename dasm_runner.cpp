#include <switch.h>
#include <cstdio>
#include <cstdint>

// forward declaration of your disassembler functions
extern "C" {
    unsigned int dasm2100(char *buffer, unsigned int op);
}

int main() {
    consoleInit(NULL);

    // Example opcodes to test your disassembler
    uint32_t opcodes[] = {
        0x00000000, // NOP
        0x02000000, // modify flag out
        0x03000001, // CALL or JUMP
        0x04100010, // stack control
        0x20012345, // ALU/MAC
        0x80123456  // DM/PM memory read/write
    };

    const int num_opcodes = sizeof(opcodes)/sizeof(opcodes[0]);
    char buffer[256]; // disassembly buffer

    for (int i = 0; i < num_opcodes; ++i) {
        dasm2100(buffer, opcodes[i]);
        printf("%08X : %s\n", opcodes[i], buffer);
    }

    printf("\nPress + to exit.\n");
    consoleUpdate(NULL);

    while (appletMainLoop()) {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if (kDown & KEY_PLUS) break;
        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
