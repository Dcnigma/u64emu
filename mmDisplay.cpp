#include "mmDisplay.h"
#include "mmDisplayD3D.h"  // <-- your wrapper
#include <cmath>

// Simple vertex shader (supports lighting & fog)
static const char* vertexShaderSrc = R"(
attribute vec3 aPosition;
attribute vec3 aNormal;
attribute vec2 aUV;

uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uProj;

uniform vec4 uAmbientColor;
uniform vec3 uLightDir;
uniform vec4 uLightColor;

uniform bool uUseFog;
uniform vec4 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

varying vec4 vColor;
varying vec2 vUV;
varying float vFogFactor;

void main() {
    vec4 worldPos = uWorld * vec4(aPosition, 1.0);
    vec3 worldNormal = normalize(mat3(uWorld) * aNormal);

    float diffuse = max(dot(worldNormal, -uLightDir), 0.0);
    vColor = uAmbientColor + uLightColor * diffuse;

    gl_Position = uProj * uView * worldPos;
    vUV = aUV;

    if(uUseFog) {
        float dist = length(worldPos.xyz);
        vFogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);
    } else {
        vFogFactor = 1.0;
    }
}
)";

// Simple fragment shader
static const char* fragmentShaderSrc = R"(
precision mediump float;

varying vec4 vColor;
varying vec2 vUV;
varying float vFogFactor;

uniform bool uUseFog;
uniform vec4 uFogColor;
uniform sampler2D uTexture;

void main() {
    vec4 texColor = texture2D(uTexture, vUV);
    vec4 finalColor = texColor * vColor;

    if(uUseFog) {
        finalColor = mix(uFogColor, finalColor, vFogFactor);
    }

    gl_FragColor = finalColor;
}
)";

mmDisplay::mmDisplay()
    : m_IsOpen(false), m_EGLDisplay(EGL_NO_DISPLAY),
      m_EGLSurface(EGL_NO_SURFACE), m_EGLContext(EGL_NO_CONTEXT),
      mShaderProgram(0), mVertexShader(0), mFragmentShader(0),
      mWidth(1280), mHeight(720), mFogEnabled(false)
{
    mAmbient[0]=mAmbient[1]=mAmbient[2]=0.2f; mAmbient[3]=1.0f;
    mLightDir[0]=0; mLightDir[1]=-1; mLightDir[2]=-1;
    mLightColor[0]=mLightColor[1]=mLightColor[2]=1.0f; mLightColor[3]=1.0f;
    mFogColor[0]=mFogColor[1]=mFogColor[2]=0.5f; mFogColor[3]=1.0f;
    mFogStart=1.0f; mFogEnd=100.0f;
}

mmDisplay::~mmDisplay() {
    Close();
}

GLuint mmDisplay::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        printf("Shader compile error: %s\n", log);
        return 0;
    }
    return shader;
}

bool mmDisplay::InitShaders() {
    mVertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    mFragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    if(!mVertexShader || !mFragmentShader) return false;

    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, mVertexShader);
    glAttachShader(mShaderProgram, mFragmentShader);
    glBindAttribLocation(mShaderProgram, 0, "aPosition");
    glBindAttribLocation(mShaderProgram, 1, "aNormal");
    glBindAttribLocation(mShaderProgram, 2, "aUV");
    glLinkProgram(mShaderProgram);

    GLint linked;
    glGetProgramiv(mShaderProgram, GL_LINK_STATUS, &linked);
    if(!linked) {
        char log[512];
        glGetProgramInfoLog(mShaderProgram, 512, nullptr, log);
        printf("Program link error: %s\n", log);
        return false;
    }

    uWorldLoc = glGetUniformLocation(mShaderProgram, "uWorld");
    uViewLoc = glGetUniformLocation(mShaderProgram, "uView");
    uProjLoc = glGetUniformLocation(mShaderProgram, "uProj");
    uAmbientLoc = glGetUniformLocation(mShaderProgram, "uAmbientColor");
    uLightDirLoc = glGetUniformLocation(mShaderProgram, "uLightDir");
    uLightColorLoc = glGetUniformLocation(mShaderProgram, "uLightColor");
    uUseFogLoc = glGetUniformLocation(mShaderProgram, "uUseFog");
    uFogColorLoc = glGetUniformLocation(mShaderProgram, "uFogColor");
    uFogStartLoc = glGetUniformLocation(mShaderProgram, "uFogStart");
    uFogEndLoc = glGetUniformLocation(mShaderProgram, "uFogEnd");
    uTextureLoc = glGetUniformLocation(mShaderProgram, "uTexture");

    return true;
}

int mmDisplay::Open() {
    m_EGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(m_EGLDisplay == EGL_NO_DISPLAY) { printf("EGL get display failed\n"); return -1; }
    if(!eglInitialize(m_EGLDisplay, nullptr, nullptr)) { printf("EGL init failed\n"); return -1; }

    EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE,8, EGL_GREEN_SIZE,8, EGL_BLUE_SIZE,8, EGL_ALPHA_SIZE,8,
        EGL_DEPTH_SIZE,24,
        EGL_NONE
    };
    EGLConfig config; EGLint numConfigs;
    if(!eglChooseConfig(m_EGLDisplay, configAttribs, &config,1,&numConfigs) || numConfigs<1) { printf("EGL config failed\n"); return -1; }

    m_EGLSurface = eglCreateWindowSurface(m_EGLDisplay, config, nullptr, nullptr);
    if(m_EGLSurface == EGL_NO_SURFACE) { printf("EGL surface failed\n"); return -1; }

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    m_EGLContext = eglCreateContext(m_EGLDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if(m_EGLContext == EGL_NO_CONTEXT) { printf("EGL context failed\n"); return -1; }

    if(!eglMakeCurrent(m_EGLDisplay, m_EGLSurface, m_EGLSurface, m_EGLContext)) { printf("EGL make current failed\n"); return -1; }

    if(!InitShaders()) { printf("Shader init failed\n"); return -1; }

    m_IsOpen = true;
    return 0;
}

void mmDisplay::Close() {
    if(m_EGLDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_EGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if(m_EGLContext != EGL_NO_CONTEXT) eglDestroyContext(m_EGLDisplay, m_EGLContext);
        if(m_EGLSurface != EGL_NO_SURFACE) eglDestroySurface(m_EGLDisplay, m_EGLSurface);
        eglTerminate(m_EGLDisplay);
    }
    m_EGLDisplay = EGL_NO_DISPLAY;
    m_EGLSurface = EGL_NO_SURFACE;
    m_EGLContext = EGL_NO_CONTEXT;
    m_IsOpen = false;
}

int mmDisplay::SetViewport(int x,int y,int width,int height) {
    if(!m_IsOpen) return -1;
    glViewport(x,y,width,height);
    mWidth=width; mHeight=height;
    return 0;
}

int mmDisplay::Clear(float r,float g,float b,float a) {
    if(!m_IsOpen) return -1;
    glClearColor(r,g,b,a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return 0;
}

int mmDisplay::SetRenderStateDefault() {
    if(!m_IsOpen) return -1;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    return 0;
}

int mmDisplay::BeginScene() {
    if(!m_IsOpen) return -1;
    glUseProgram(mShaderProgram);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform4fv(uAmbientLoc,1,mAmbient);
    glUniform3fv(uLightDirLoc,1,mLightDir);
    glUniform4fv(uLightColorLoc,1,mLightColor);
    glUniform1i(uUseFogLoc,mFogEnabled);
    glUniform4fv(uFogColorLoc,1,mFogColor);
    glUniform1f(uFogStartLoc,mFogStart);
    glUniform1f(uFogEndLoc,mFogEnd);

    return 0;
}

int mmDisplay::EndScene() {
    return 0;
}

int mmDisplay::Present() {
    if(!m_IsOpen) return -1;
    eglSwapBuffers(m_EGLDisplay,m_EGLSurface);
    return 0;
}

void mmDisplay::SetAmbientLight(float r,float g,float b,float a) {
    mAmbient[0]=r; mAmbient[1]=g; mAmbient[2]=b; mAmbient[3]=a;
}

void mmDisplay::SetLightDirection(float x,float y,float z) {
    mLightDir[0]=x; mLightDir[1]=y; mLightDir[2]=z;
}

void mmDisplay::SetLightColor(float r,float g,float b,float a) {
    mLightColor[0]=r; mLightColor[1]=g; mLightColor[2]=b; mLightColor[3]=a;
}

void mmDisplay::EnableFog(bool enable,float start,float end,float r,float g,float b,float a) {
    mFogEnabled = enable;
    mFogStart = start; mFogEnd = end;
    mFogColor[0]=r; mFogColor[1]=g; mFogColor[2]=b; mFogColor[3]=a;
}
int mmDisplay::DrawIndexedPrimitive(D3DVertex* vertices, int vertexCount,
                                    unsigned short* indices, int indexCount)
{
    if(!m_IsOpen) return -1;
    if(!vertices || !indices || vertexCount<=0 || indexCount<=0) return -1;

    // Create temporary VBOs
    GLuint vbo, ibo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(D3DVertex), vertices, GL_DYNAMIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_DYNAMIC_DRAW);

    // Enable attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(D3DVertex), (void*)0);

    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(D3DVertex), (void*)(3*sizeof(float)));

    glEnableVertexAttribArray(2); // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(D3DVertex), (void*)(6*sizeof(float)));

    // Draw
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);

    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);

    return 0;
}
// Set world, view, projection matrices
void mmDisplay::SetTransform(const float* world, const float* view, const float* proj)
{
    if(!m_IsOpen) return;
    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(uWorldLoc, 1, GL_FALSE, world);
    glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, view);
    glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, proj);
}

// Bind texture to shader
void mmDisplay::BindTexture(GLuint texture)
{
    if(!m_IsOpen) return;
    glUseProgram(mShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(uTextureLoc, 0);
}

// Helper: create a look-at matrix (similar to D3D's LookAtLH)
static void LookAtLH(float* matrix, const float* eye, const float* target, const float* up)
{
    float zaxis[3] = { target[0]-eye[0], target[1]-eye[1], target[2]-eye[2] };
    float len = std::sqrt(zaxis[0]*zaxis[0]+zaxis[1]*zaxis[1]+zaxis[2]*zaxis[2]);
    zaxis[0]/=len; zaxis[1]/=len; zaxis[2]/=len;

    float xaxis[3] = {
        up[1]*zaxis[2] - up[2]*zaxis[1],
        up[2]*zaxis[0] - up[0]*zaxis[2],
        up[0]*zaxis[1] - up[1]*zaxis[0]
    };
    len = std::sqrt(xaxis[0]*xaxis[0]+xaxis[1]*xaxis[1]+xaxis[2]*xaxis[2]);
    xaxis[0]/=len; xaxis[1]/=len; xaxis[2]/=len;

    float yaxis[3] = {
        zaxis[1]*xaxis[2] - zaxis[2]*xaxis[1],
        zaxis[2]*xaxis[0] - zaxis[0]*xaxis[2],
        zaxis[0]*xaxis[1] - zaxis[1]*xaxis[0]
    };

    matrix[0] = xaxis[0]; matrix[1] = yaxis[0]; matrix[2] = zaxis[0]; matrix[3] = 0.0f;
    matrix[4] = xaxis[1]; matrix[5] = yaxis[1]; matrix[6] = zaxis[1]; matrix[7] = 0.0f;
    matrix[8] = xaxis[2]; matrix[9] = yaxis[2]; matrix[10]= zaxis[2]; matrix[11]= 0.0f;
    matrix[12] = - (xaxis[0]*eye[0] + xaxis[1]*eye[1] + xaxis[2]*eye[2]);
    matrix[13] = - (yaxis[0]*eye[0] + yaxis[1]*eye[1] + yaxis[2]*eye[2]);
    matrix[14] = - (zaxis[0]*eye[0] + zaxis[1]*eye[1] + zaxis[2]*eye[2]);
    matrix[15] = 1.0f;
}

// Implementation
void mmDisplay::SetCamera(const float* position, const float* target, const float* up)
{
    if(!m_IsOpen) return;
    float view[16];
    LookAtLH(view, position, target, up);
    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, view);
}
// Helper: create a perspective projection matrix (like D3D PerspectiveFovLH)
static void PerspectiveFovLH(float* matrix, float fovY, float aspect, float zNear, float zFar)
{
    float yScale = 1.0f / tanf(fovY * 0.5f);
    float xScale = yScale / aspect;
    float zRange = zFar - zNear;

    matrix[0] = xScale; matrix[1] = 0;      matrix[2] = 0;                    matrix[3] = 0;
    matrix[4] = 0;      matrix[5] = yScale; matrix[6] = 0;                    matrix[7] = 0;
    matrix[8] = 0;      matrix[9] = 0;      matrix[10] = zFar / zRange;       matrix[11] = 1;
    matrix[12] = 0;     matrix[13] = 0;     matrix[14] = -zNear * zFar / zRange; matrix[15] = 0;
}

void mmDisplay::SetProjectionFov(float fovY, float aspect, float zNear, float zFar)
{
    if(!m_IsOpen) return;
    float proj[16];
    PerspectiveFovLH(proj, fovY, aspect, zNear, zFar);
    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, proj);
}

GLuint mmDisplay::CreateTexture(int width, int height, const unsigned char* data)
{
    if (!m_IsOpen || !data) return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Upload RGBA data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Basic filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return tex;
}

void mmDisplay::SetTexture(GLuint texture)
{
    if (!m_IsOpen) return;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(mShaderProgram);
    glUniform1i(uTextureLoc, 0); // Shader sampler at slot 0
}

void mmDisplay::ReleaseTexture(GLuint texture)
{
    if (texture != 0)
        glDeleteTextures(1, &texture);
}
