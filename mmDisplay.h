#pragma once

#include <switch.h>
#include <GLES2/gl2.h>
#include <cstdio>
#include <cstring>

class mmDisplay
{
public:
    mmDisplay();
    ~mmDisplay();

    int Open();
    void Close();
    int SetViewport(int x, int y, int width, int height);
    int Clear(float r, float g, float b, float a);
    int SetRenderStateDefault();
    int BeginScene();
    int EndScene();
    int Present();

    void SetAmbientLight(float r, float g, float b, float a);
    void SetLightDirection(float x, float y, float z);
    void SetLightColor(float r, float g, float b, float a);
    void EnableFog(bool enable, float start, float end, float r, float g, float b, float a);

    void SetTransform(const float* world, const float* view, const float* proj);
    void BindTexture(GLuint texture);

    void SetCamera(const float* position, const float* target, const float* up);
    void SetProjectionFov(float fovY, float aspect, float zNear, float zFar);

    GLuint CreateTexture(int width, int height, const unsigned char* data);
    void SetTexture(GLuint texture);
    void ReleaseTexture(GLuint texture);

private:
    bool m_IsOpen;
    int m_Width, m_Height;

    GLuint mShaderProgram;
    GLuint mVertexShader;
    GLuint mFragmentShader;

    GLint uWorldLoc;
    GLint uViewLoc;
    GLint uProjLoc;
    GLint uAmbientLoc;
    GLint uLightDirLoc;
    GLint uLightColorLoc;
    GLint uUseFogLoc;
    GLint uFogColorLoc;
    GLint uFogStartLoc;
    GLint uFogEndLoc;
    GLint uTextureLoc;

    float mAmbient[4];
    float mLightDir[3];
    float mLightColor[4];
    bool mFogEnabled;
    float mFogColor[4];
    float mFogStart, mFogEnd;

    GLuint CompileShader(GLenum type, const char* source);
    bool InitShaders();

    struct D3DVertex {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
    };
};
