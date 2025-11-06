#pragma once
#include <switch.h>
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
    void UpdateScreenBuffer(unsigned char* source);
    void MakeScreenBuffer();
    void ClearScreen();
    void BeginScene();
    void EndScene();

private:
    static EGLDisplay s_display;
    static EGLContext s_context;
    static EGLSurface s_surface;
    static GLuint s_program;
    static GLuint VBO, VAO, EBO;
    static GLuint s_tex;
};

// Declare global display object
extern mmDisplay theDisplay;
