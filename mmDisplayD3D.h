#pragma once
#include "mmDisplay.h"
#include <vector>
#include <map>
#include <cstring>
#include <cmath>

// --- Shader wrapper ---
class ShaderProgram {
public:
    ShaderProgram(GLuint program=0) : mProgram(program) {}
    void Use() { if(mProgram) glUseProgram(mProgram); }
    GLuint GetProgram() const { return mProgram; }
private:
    GLuint mProgram;
};

// --- Vertex & Index Buffers ---
class D3DVertexBuffer {
public:
    D3DVertexBuffer(D3DVertex* verts,int count,bool dynamic=false)
        : vertexCount(count), isDynamic(dynamic)
    {
        data.assign(verts, verts+count);
    }

    void* Lock() { return data.data(); }
    void Unlock() {}

    const D3DVertex* GetData() const { return data.data(); }
    int GetCount() const { return vertexCount; }

private:
    std::vector<D3DVertex> data;
    int vertexCount;
    bool isDynamic;
};

class D3DIndexBuffer {
public:
    D3DIndexBuffer(unsigned short* inds,int count,bool dynamic=false)
        : indexCount(count), isDynamic(dynamic)
    {
        data.assign(inds, inds+count);
    }

    void* Lock() { return data.data(); }
    void Unlock() {}

    const unsigned short* GetData() const { return data.data(); }
    int GetCount() const { return indexCount; }

private:
    std::vector<unsigned short> data;
    int indexCount;
    bool isDynamic;
};

// --- Material / Render State wrapper ---
struct Material {
    ShaderProgram* shader;
    float ambient[4];
    float diffuse[4];
    float specular[4];
    bool useFog;
    float fogColor[4];
    float fogStart, fogEnd;
    bool alphaBlend;
    bool cullBack;
    std::map<int, GLuint> textures; // stage -> texture
};

// --- D3D-style device wrapper ---
class mmDisplayD3D {
public:
    mmDisplay Display;

    mmDisplayD3D() { ResetStates(); }

    // Matrices
    void SetWorld(const float* mat) { memcpy(mWorld, mat, sizeof(float)*16); UpdateMatrices(); }
    void SetView(const float* mat)  { memcpy(mView, mat, sizeof(float)*16); UpdateMatrices(); }
    void SetProjection(const float* mat) { memcpy(mProj, mat, sizeof(float)*16); UpdateMatrices(); }

    // Scene management
    void Clear(float r,float g,float b,float a) { Display.Clear(r,g,b,a); }
    void BeginScene() { Display.BeginScene(); ApplyMaterial(); }
    void EndScene() { Display.EndScene(); }
    void Present() { Display.Present(); }

    // Lighting
    void SetAmbient(float r,float g,float b,float a) { mCurrentMaterial.ambient[0]=r; mCurrentMaterial.ambient[1]=g; mCurrentMaterial.ambient[2]=b; mCurrentMaterial.ambient[3]=a; Display.SetAmbientLight(r,g,b,a); }
    void SetLightColor(float r,float g,float b,float a) { mCurrentMaterial.diffuse[0]=r; mCurrentMaterial.diffuse[1]=g; mCurrentMaterial.diffuse[2]=b; mCurrentMaterial.diffuse[3]=a; Display.SetLightColor(r,g,b,a); }
    void SetLightDir(float x,float y,float z) { mCurrentMaterial.shader ? Display.SetLightDirection(x,y,z) : Display.SetLightDirection(x,y,z); }

    void EnableFog(bool enable,float start,float end,float r,float g,float b,float a) { mCurrentMaterial.useFog=enable; mCurrentMaterial.fogStart=start; mCurrentMaterial.fogEnd=end; mCurrentMaterial.fogColor[0]=r; mCurrentMaterial.fogColor[1]=g; mCurrentMaterial.fogColor[2]=b; mCurrentMaterial.fogColor[3]=a; Display.EnableFog(enable,start,end,r,g,b,a); }

    // Render states
    void SetAlphaBlend(bool enable) { mCurrentMaterial.alphaBlend=enable; if(enable) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); } else glDisable(GL_BLEND); }
    void SetCullBackFace(bool enable) { mCurrentMaterial.cullBack=enable; if(enable) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); } else glDisable(GL_CULL_FACE); }

    // --- Shader / Material ---
    void SetShader(ShaderProgram* shader) { mCurrentMaterial.shader=shader; if(shader) shader->Use(); }

    // --- Multi-texture support ---
    void SetTexture(int stage, GLuint tex) {
        if(stage<0 || stage>7) return;
        glActiveTexture(GL_TEXTURE0 + stage);
        glBindTexture(GL_TEXTURE_2D, tex);
        mCurrentMaterial.textures[stage] = tex;
        if(mCurrentMaterial.shader) glUniform1i(Display.uTextureLoc, stage);
    }

    // --- Draw using buffers ---
    int DrawIndexedPrimitive(D3DVertexBuffer* vbuf,D3DIndexBuffer* ibuf) {
        if(!vbuf || !ibuf) return -1;
        return Display.DrawIndexedPrimitive((D3DVertex*)vbuf->GetData(), vbuf->GetCount(),
                                            (unsigned short*)ibuf->GetData(), ibuf->GetCount());
    }

    GLuint CreateTexture(int width,int height,const unsigned char* data) { return Display.CreateTexture(width,height,data); }
    void ReleaseTexture(GLuint tex) { Display.ReleaseTexture(tex); }

private:
    float mWorld[16], mView[16], mProj[16];
    Material mCurrentMaterial;

    void ResetStates() {
        memset(mWorld,0,sizeof(mWorld));
        memset(mView,0,sizeof(mView));
        memset(mProj,0,sizeof(mProj));
        for(int i=0;i<16;i++) mWorld[i]=mView[i]=mProj[i]=0;
        mWorld[0]=mWorld[5]=mWorld[10]=mWorld[15]=1.0f;
        mView[0]=mView[5]=mView[10]=mView[15]=1.0f;
        mProj[0]=mProj[5]=mProj[10]=mProj[15]=1.0f;
        mCurrentMaterial.shader=nullptr;
        mCurrentMaterial.alphaBlend=false;
        mCurrentMaterial.cullBack=true;
        mCurrentMaterial.useFog=false;
    }

    void UpdateMatrices() { Display.SetTransform(mWorld,mView,mProj); }

    void ApplyMaterial() {
        if(mCurrentMaterial.shader) mCurrentMaterial.shader->Use();
        Display.SetAmbientLight(mCurrentMaterial.ambient[0],mCurrentMaterial.ambient[1],mCurrentMaterial.ambient[2],mCurrentMaterial.ambient[3]);
        Display.SetLightColor(mCurrentMaterial.diffuse[0],mCurrentMaterial.diffuse[1],mCurrentMaterial.diffuse[2],mCurrentMaterial.diffuse[3]);
        Display.EnableFog(mCurrentMaterial.useFog,mCurrentMaterial.fogStart,mCurrentMaterial.fogEnd,
                          mCurrentMaterial.fogColor[0],mCurrentMaterial.fogColor[1],mCurrentMaterial.fogColor[2],mCurrentMaterial.fogColor[3]);
        SetAlphaBlend(mCurrentMaterial.alphaBlend);
        SetCullBackFace(mCurrentMaterial.cullBack);

        for(auto& kv : mCurrentMaterial.textures)
            SetTexture(kv.first, kv.second);
    }
};
