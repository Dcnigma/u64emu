#pragma once
#include <switch.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cstdio>
#include <cstring>

class mmDisplay
{
public:
    mmDisplay();
    ~mmDisplay();

    // Core
    int Open();
    void Close();
    int SetViewport(int x, int y, int width, int height);
    int Clear(float r, float g, float b, float a);
    int SetRenderStateDefault();
    int BeginScene();
    int EndScene();
    int Present();

    // D3D-like replacements
    void SetAmbientLight(float r, float g, float b, float a);
    void SetLightDirection(float x, float y, float z);
    void SetLightColor(float r, float g, float b, float a);
    void EnableFog(bool enable, float start, float end, float r, float g, float b, float a);

    // New helpers
    void SetTransform(const float* world, const float* view, const float* proj);
    void BindTexture(GLuint texture);

		// D3D-like camera
		void SetCamera(const float* position, const float* target, const float* up);

		// D3D-like projection
		void SetProjectionFov(float fovY, float aspect, float zNear, float zFar);
		
		// Texture management (D3D-like)
    GLuint CreateTexture(int width, int height, const unsigned char* data);
    void SetTexture(GLuint texture);
    void ReleaseTexture(GLuint texture);
private:
    // EGL
    EGLDisplay m_EGLDisplay;
    EGLSurface m_EGLSurface;
    EGLContext m_EGLContext;
    bool m_IsOpen;

    int m_Width, m_Height;

    // Shader program
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

    // Lighting/fog state
    float mAmbient[4];
    float mLightDir[3];
    float mLightColor[4];
    bool mFogEnabled;
    float mFogColor[4];
    float mFogStart, mFogEnd;

    // Simple vertex struct similar to D3DVERTEX
    struct D3DVertex {
        float x, y, z;       // position
        float nx, ny, nz;    // normal
        float u, v;          // texture coords
    };

    // Draw function (similar to D3D DrawIndexedPrimitive)
    int DrawIndexedPrimitive(D3DVertex* vertices, int vertexCount,
                             unsigned short* indices, int indexCount);

private:
    GLuint CompileShader(GLenum type, const char* source);
    bool InitShaders();
};
