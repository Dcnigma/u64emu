#include "mmDisplay.h"
#include <switch.h>
#include <EGL/egl.h>
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

mmDisplay theDisplay;

mmDisplay::mmDisplay() {
    IntermediateBuffer = (WORD*)malloc(320 * 240 * sizeof(WORD));
    nwindowSetDimensions(nwindowGetDefault(), 1280, 720);
}

mmDisplay::~mmDisplay() {
    if (IntermediateBuffer) free(IntermediateBuffer);
}

int mmDisplay::Open(uint16_t Width, uint16_t Height) {
    // Minimal EGL/OpenGL init
    return 0;
}

void mmDisplay::Close() {}

bool mmDisplay::RenderScene() { return true; }
void mmDisplay::ClearScreen() {}
void mmDisplay::BeginScene() {}
void mmDisplay::EndScene() {}

void mmDisplay::UpdateScreenBuffer(unsigned char* source) {
    // Fill buffer with a solid color for testing
    for (int i = 0; i < 320 * 240; i++) {
        IntermediateBuffer[i] = 0xF800; // Red
    }
}

void mmDisplay::MakeScreenBuffer() {}
