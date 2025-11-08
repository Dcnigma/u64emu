#include <switch.h>
#include <GLES2/gl2.h>
#include <cstdio>
#include <cmath>
#include "mmDisplay.h"

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

// ------------------------------------------
// mmDisplay methods
// ------------------------------------------

mmDisplay::mmDisplay()
    : m_IsOpen(false),
      mShaderProgram(0), mVertexShader(0), mFragmentShader(0),
      mWidth(1280), mHeight(720),
      mFogEnabled(false)
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
    gfxInitDefault();
    consoleInit(NULL);

    if(!InitShaders()) {
        printf("Shader init failed\n");
        return -1;
    }

    m_IsOpen = true;
    return 0;
}

void mmDisplay::Close() {
    if(!m_IsOpen) return;
    gfxExit();
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
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    return 0;
}

// -------------------------------
// Shader helpers
// -------------------------------
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

// -------------------------------
// Draw helpers
// -------------------------------
int mmDisplay::DrawIndexedPrimitive(D3DVertex* vertices, int vertexCount,
                                    unsigned short* indices, int indexCount)
{
    if(!m_IsOpen || !vertices || !indices || vertexCount<=0 || indexCount<=0) return -1;

    GLuint vbo, ibo;
    glGenBuffers(1,&vbo);
    glGenBuffers(1,&ibo);

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeof(D3DVertex), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned short), indices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(D3DVertex),(void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(D3DVertex),(void*)(3*sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(D3DVertex),(void*)(6*sizeof(float)));

    glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_SHORT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glDeleteBuffers(1,&vbo);
    glDeleteBuffers(1,&ibo);

    return 0;
}

// -------------------------------
// Transform / Camera
// -------------------------------
void mmDisplay::SetTransform(const float* world, const float* view, const float* proj) {
    if(!m_IsOpen) return;
    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(uWorldLoc,1,GL_FALSE,world);
    glUniformMatrix4fv(uViewLoc,1,GL_FALSE,view);
    glUniformMatrix4fv(uProjLoc,1,GL_FALSE,proj);
}

void mmDisplay::BindTexture(GLuint texture) {
    if(!m_IsOpen) return;
    glUseProgram(mShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(uTextureLoc,0);
}

static void LookAtLH(float* matrix, const float* eye, const float* target, const float* up) {
    float z[3] = { target[0]-eye[0], target[1]-eye[1], target[2]-eye[2] };
    float len = std::sqrt(z[0]*z[0]+z[1]*z[1]+z[2]*z[2]);
    z[0]/=len; z[1]/=len; z[2]/=len;

    float x[3] = { up[1]*z[2]-up[2]*z[1], up[2]*z[0]-up[0]*z[2], up[0]*z[1]-up[1]*z[0] };
    len = std::sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
    x[0]/=len; x[1]/=len; x[2]/=len;

    float y[3] = { z[1]*x[2]-z[2]*x[1], z[2]*x[0]-z[0]*x[2], z[0]*x[1]-z[1]*x[0] };

    matrix[0]=x[0]; matrix[1]=y[0]; matrix[2]=z[0]; matrix[3]=0.0f;
    matrix[4]=x[1]; matrix[5]=y[1]; matrix[6]=z[1]; matrix[7]=0.0f;
    matrix[8]=x[2]; matrix[9]=y[2]; matrix[10]=z[2]; matrix[11]=0.0f;
    matrix[12]=-(x[0]*eye[0]+x[1]*eye[1]+x[2]*eye[2]);
    matrix[13]=-(y[0]*eye[0]+y[1]*eye[1]+y[2]*eye[2]);
    matrix[14]=-(z[0]*eye[0]+z[1]*eye[1]+z[2]*eye[2]);
    matrix[15]=1.0f;
}

void mmDisplay::SetCamera(const float* position, const float* target, const float* up) {
    if(!m_IsOpen) return;
    float view[16];
    LookAtLH(view, position, target, up);
    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(uViewLoc,1,GL_FALSE,view);
}

static void PerspectiveFovLH(float* matrix, float fovY, float aspect, float zNear, float zFar) {
    float yScale = 1.0f / tanf(fovY*0.5f);
    float xScale = yScale / aspect;
    float zRange = zFar-zNear;

    matrix[0]=xScale; matrix[1]=0; matrix[2]=0; matrix[3]=0;
    matrix[4]=0; matrix[5]=yScale; matrix[6]=0; matrix[7]=0;
    matrix[8]=0; matrix[9]=0; matrix[10]=zFar/zRange; matrix[11]=1;
    matrix[12]=0; matrix[13]=0; matrix[14]=-zNear*zFar/zRange; matrix[15]=0;
}

void mmDisplay::SetProjectionFov(float fovY, float aspect, float zNear, float zFar) {
    if(!m_IsOpen) return;
    float proj[16];
    PerspectiveFovLH(proj,fovY,aspect,zNear,zFar);
    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(uProjLoc,1,GL_FALSE,proj);
}

GLuint mmDisplay::CreateTexture(int width, int height, const unsigned char* data) {
    if(!m_IsOpen || !data) return 0;

    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    return tex;
}

void mmDisplay::SetTexture(GLuint texture) {
    if(!m_IsOpen) return;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texture);
    glUseProgram(mShaderProgram);
    glUniform1i(uTextureLoc,0);
}

void mmDisplay::ReleaseTexture(GLuint texture) {
    if(texture!=0)
        glDeleteTextures(1,&texture);
}
