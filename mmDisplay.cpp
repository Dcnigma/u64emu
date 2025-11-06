#include "mmDisplay.h"
#include "global.h"
#include "ki.h"

#include <switch.h>
#include <EGL/egl.h>
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 640
#define HEIGHT 480

//-----------------------------------------------------------------------------
// EGL/OpenGL
//-----------------------------------------------------------------------------
static EGLDisplay s_display = EGL_NO_DISPLAY;
static EGLContext s_context = EGL_NO_CONTEXT;
static EGLSurface s_surface = EGL_NO_SURFACE;

static GLuint s_program;
static GLuint VBO, VAO, EBO;
static GLuint s_tex;

static const float vertices[] = {
    // positions       // colors        // tex coords
     0.76f,  1.0f, 0.0f, 1.0f,0.0f,0.0f, 1.0f,0.0f,
     0.76f, -1.0f, 0.0f, 0.0f,1.0f,0.0f, 1.0f,1.0f,
    -0.76f, -1.0f, 0.0f, 0.0f,0.0f,1.0f, 0.0f,1.0f,
    -0.76f,  1.0f, 0.0f, 1.0f,1.0f,0.0f, 0.0f,0.0f
};

static const unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

// Shader sources
static const char* vertexShaderSource = R"text(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;
out vec3 ourColor;
out vec2 TexCoord;
void main() { gl_Position = vec4(aPos,1.0); ourColor = aColor; TexCoord = aTexCoord; }
)text";

static const char* fragmentShaderSource = R"text(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() { FragColor = vec4(ourColor,1.0); }
)text";

//-----------------------------------------------------------------------------
// mmDisplay implementation
//-----------------------------------------------------------------------------
mmDisplay theDisplay;

mmDisplay::mmDisplay() {
    IntermediateBuffer = (WORD*)malloc(320*240*sizeof(WORD));
    nwindowSetDimensions(nwindowGetDefault(), 1280, 720);
}

mmDisplay::~mmDisplay() {
    if (IntermediateBuffer) free(IntermediateBuffer);
}

static GLuint createAndCompileShader(GLenum type, const char* source) {
    GLuint handle = glCreateShader(type);
    glShaderSource(handle,1,&source,NULL);
    glCompileShader(handle);
    GLint success;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buf[512];
        glGetShaderInfoLog(handle, sizeof(buf), NULL, buf);
        return 0;
    }
    return handle;
}

bool initEgl() {
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (s_display == EGL_NO_DISPLAY) return false;
    eglInitialize(s_display,NULL,NULL);
    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) return false;

    EGLConfig config;
    EGLint numConfigs;
    static const EGLint framebufferAttributeList[] = {
        EGL_RED_SIZE,1, EGL_GREEN_SIZE,1, EGL_BLUE_SIZE,1, EGL_NONE
    };
    eglChooseConfig(s_display, framebufferAttributeList, &config,1,&numConfigs);
    s_surface = eglCreateWindowSurface(s_display, config, (char*)"", NULL);
    if (!s_surface) return false;

    static const EGLint contextAttributeList[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,  // Switch supports up to OpenGL 3.x
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE
    };
    s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
    if (!s_context) return false;

    eglMakeCurrent(s_display, s_surface, s_surface, s_context);
    return true;
}

void deinitEgl() {
    if (s_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (s_context) { eglDestroyContext(s_display,s_context); s_context = EGL_NO_CONTEXT; }
        if (s_surface) { eglDestroySurface(s_display,s_surface); s_surface = EGL_NO_SURFACE; }
        eglTerminate(s_display);
        s_display = EGL_NO_DISPLAY;
    }
}

int mmDisplay::Open(uint16_t Width, uint16_t Height) {
    if (!initEgl()) return -1;
    gladLoadGL();

    GLuint vsh = createAndCompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fsh = createAndCompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    s_program = glCreateProgram();
    glAttachShader(s_program, vsh);
    glAttachShader(s_program, fsh);
    glLinkProgram(s_program);
    glDeleteShader(vsh);
    glDeleteShader(fsh);

    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenTextures(1,&s_tex);
    glBindTexture(GL_TEXTURE_2D,s_tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    return 0;
}

void mmDisplay::Close() { deinitEgl(); }

bool mmDisplay::RenderScene() {
    // Draw test color
    glClearColor(0.1f,0.3f,0.7f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(s_display,s_surface);
    return true;
}

void mmDisplay::ClearScreen() { glClear(GL_COLOR_BUFFER_BIT); }
void mmDisplay::BeginScene() {}
void mmDisplay::EndScene() {}

void mmDisplay::UpdateScreenBuffer(unsigned char* source) {
    // For now just fill buffer with red
    for (int i=0;i<320*240;i++) IntermediateBuffer[i]=0xF800;
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB565,320,240,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,IntermediateBuffer);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    eglSwapBuffers(s_display,s_surface);
}

void mmDisplay::MakeScreenBuffer() {}
