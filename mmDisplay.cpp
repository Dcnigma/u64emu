#include "mmDisplay.h"
#include "global.h"
#include "ki.h"

#include <switch.h>
#include <EGL/egl.h>
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH  1280
#define HEIGHT 720

//-----------------------------------------------------------------------------
// EGL/OpenGL static members
//-----------------------------------------------------------------------------
EGLDisplay mmDisplay::s_display = EGL_NO_DISPLAY;
EGLContext mmDisplay::s_context = EGL_NO_CONTEXT;
EGLSurface mmDisplay::s_surface = EGL_NO_SURFACE;
GLuint mmDisplay::s_program = 0;
GLuint mmDisplay::VBO = 0;
GLuint mmDisplay::VAO = 0;
GLuint mmDisplay::EBO = 0;
GLuint mmDisplay::s_tex = 0;

//-----------------------------------------------------------------------------
// Vertex and fragment shader data
//-----------------------------------------------------------------------------
static const float vertices[] = {
    // positions       // colors        // tex coords
     1.0f,  1.0f, 0.0f, 1.0f,0.0f,0.0f, 1.0f,0.0f,
     1.0f, -1.0f, 0.0f, 0.0f,1.0f,0.0f, 1.0f,1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,0.0f,1.0f, 0.0f,1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,1.0f,0.0f, 0.0f,0.0f
};

static const unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

static const char* vertexShaderSource = R"text(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)text";

static const char* fragmentShaderSource = R"text(
#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoord);
}
)text";

//-----------------------------------------------------------------------------
// Global display object
//-----------------------------------------------------------------------------
mmDisplay theDisplay;

//-----------------------------------------------------------------------------
// mmDisplay implementation
//-----------------------------------------------------------------------------
mmDisplay::mmDisplay() {
    IntermediateBuffer = (WORD*)malloc(320 * 240 * sizeof(WORD));
    nwindowSetDimensions(nwindowGetDefault(), WIDTH, HEIGHT);
}

mmDisplay::~mmDisplay() {
    if (IntermediateBuffer) free(IntermediateBuffer);
}

//-----------------------------------------------------------------------------
// Private methods for EGL initialization
//-----------------------------------------------------------------------------
bool mmDisplay::initEgl() {
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (s_display == EGL_NO_DISPLAY) return false;

    eglInitialize(s_display, NULL, NULL);
    eglBindAPI(EGL_OPENGL_API);

    EGLConfig config;
    EGLint numConfigs;
    static const EGLint framebufferAttributeList[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE
    };
    eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);

    s_surface = eglCreateWindowSurface(s_display, config, (NativeWindowType)nwindowGetDefault(), NULL);
    if (s_surface == EGL_NO_SURFACE) return false;

    static const EGLint contextAttributeList[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_NONE
    };
    s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
    if (s_context == EGL_NO_CONTEXT) return false;

    eglMakeCurrent(s_display, s_surface, s_surface, s_context);
    return true;
}

void mmDisplay::deinitEgl() {
    if (s_display != EGL_NO_DISPLAY) {
        if (s_context) eglDestroyContext(s_display, s_context);
        if (s_surface) eglDestroySurface(s_display, s_surface);
        eglTerminate(s_display);

        s_display = EGL_NO_DISPLAY;
        s_context = EGL_NO_CONTEXT;
        s_surface = EGL_NO_SURFACE;
    }
}

//-----------------------------------------------------------------------------
// Shader helper
//-----------------------------------------------------------------------------
static GLuint createAndCompileShader(GLenum type, const char* source) {
    GLuint handle = glCreateShader(type);
    glShaderSource(handle, 1, &source, NULL);
    glCompileShader(handle);

    GLint success;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buf[512];
        glGetShaderInfoLog(handle, sizeof(buf), NULL, buf);
        printf("Shader compile error: %s\n", buf);
        return 0;
    }
    return handle;
}

//-----------------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------------
int mmDisplay::Open(uint16_t Width, uint16_t Height) {
    if (!initEgl()) return -1;

    gladLoadGL();
    glViewport(0, 0, WIDTH, HEIGHT);

    GLuint vsh = createAndCompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fsh = createAndCompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    s_program = glCreateProgram();
    glAttachShader(s_program, vsh);
    glAttachShader(s_program, fsh);
    glLinkProgram(s_program);
    glDeleteShader(vsh);
    glDeleteShader(fsh);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenTextures(1, &s_tex);
    glBindTexture(GL_TEXTURE_2D, s_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUseProgram(s_program);
    glUniform1i(glGetUniformLocation(s_program, "texture1"), 0);

    return 0;
}

void mmDisplay::Close() { deinitEgl(); }

bool mmDisplay::RenderScene() {
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(s_program);
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_tex);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    eglSwapBuffers(s_display, s_surface);

    return true;
}

void mmDisplay::ClearScreen() { glClear(GL_COLOR_BUFFER_BIT); }
void mmDisplay::BeginScene() {}
void mmDisplay::EndScene() {}

void mmDisplay::UpdateScreenBuffer(unsigned char* source) {
    for (int i = 0; i < 320 * 240; i++) IntermediateBuffer[i] = 0xF800;

    glBindTexture(GL_TEXTURE_2D, s_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 320, 240, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, IntermediateBuffer);

    glUseProgram(s_program);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    eglSwapBuffers(s_display, s_surface);
}

void mmDisplay::MakeScreenBuffer() {}
