#pragma once
#include <stdint.h>

class CEmuObject {
public:
    bool m_Debug = false;

    void Init() {
        // Initialize emulator state
    }

    void Emulate(const char* romName) {
        // Start CPU/DSP loop (stub)
    }

    void StopEmulation() {
        // Stop CPU/DSP loop
    }

    void UpdateDisplay(); // implemented in mmDisplay
    void UpdateAudio(uint32_t size) {
        // Stub: no sound yet
    }
};
