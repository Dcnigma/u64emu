#pragma once
#include <stdint.h>
#include <EGL/egl.h>
#include <glad/glad.h>

typedef unsigned short WORD;

class mmDisplay {
public:
    mmDisplay();
    ~mmDisplay();

    int Open(uint16_t Width, uint16_t Height);
    void Close();
    bool RenderScene();
    void ClearScreen();
    void BeginScene();
    void EndScene();
    void UpdateScreenBuffer(unsigned char* source);
    void MakeScreenBuffer();

private:
    WORD* IntermediateBuffer;
};

extern mmDisplay theDisplay;
