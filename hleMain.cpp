#include <vector>
#include <switch.h>
#include <cmath>

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <queue>
#include <string>
#include <filesystem>
#include <fstream>

#include "hleMain.h"
#include "iMemory.h"
#include "dynaNative.h"
#include "dynaFP.h"
#include "hleDSP.h"
#include "iCPU.h"
#include "ki.h"

// -------------------------- Globals --------------------------
extern WORD inputs[4];
bool hleFMVDelay = false;

float ident[16] = {
    1.f,0.f,0.f,0.f,
    0.f,1.f,0.f,0.f,
    0.f,0.f,1.f,0.f,
    0.f,0.f,0.f,1.f
};

WORD identl[32] = {
    0,1,0,0,
    1,0,0,0,
    0,0,0,1,
    0,0,1,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
};

// -------------------------- Memory / DMA --------------------------
void hleICache() {
    DWORD addr = r->GPR[A0*2];
    DWORD len = r->GPR[A1*2];
    dynaInvalidate(addr, len);
}

void hleVirtualToPhysical() {
    DWORD addr = r->GPR[A0*2];
    if(addr < 0x00400000) addr |= 0x80000000;
    else if(addr < 0x05000000) addr |= 0xa0000000;
    else addr &= 0x3fffff;
    r->GPR[V0*2] = addr;
}

void hlePiRawStartDma() { }
void hlePiStartDma()    { }

void hleCos() { *(float*)&r->FPR[0] = std::cos(*(float*)&r->FPR[12]); }
void hleSin() { *(float*)&r->FPR[0] = std::sin(*(float*)&r->FPR[12]); }

void hleNormalize(float* vec) {
    float len = std::sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
    if(len != 0.f) {
        vec[0] /= len; vec[1] /= len; vec[2] /= len;
    }
}

void hleMtxIdentF(float* m) { std::memcpy(m, ident, sizeof(ident)); }
void hleMtxIdent(WORD* m) { std::memcpy(m, identl, sizeof(identl)); }
void hleMtxCopyF(float* dst, float* src) { std::memcpy(dst, src, sizeof(float)*16); }
void hleMtxCopy(WORD* dst, WORD* src) { std::memcpy(dst, src, sizeof(WORD)*32); }

void hleVecCross(float* dst, float* a, float* b) {
    dst[0] = a[1]*b[2] - a[2]*b[1];
    dst[1] = a[2]*b[0] - a[0]*b[2];
    dst[2] = a[0]*b[1] - a[1]*b[0];
}

float hleVecDot(float* a, float* b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
void hleVecScale(float* dst, float* v, float s) { dst[0]=v[0]*s; dst[1]=v[1]*s; dst[2]=v[2]*s; }

void hleMemSet(void* dst, int val, size_t size) { std::memset(dst, val, size); }
void hleMemCpy(void* dst, const void* src, size_t size) { std::memcpy(dst, src, size); }
void hleDmaCopy(void* dst, const void* src, size_t size) { hleMemCpy(dst, src, size); }
void* hleAlloc(size_t size) { return malloc(size); }
void hleFree(void* ptr) { if(ptr) free(ptr); }

// -------------------------- Input HLE --------------------------
InputHLE_Extended gInputHLE(2); // Two controllers

void hleKeyInput() {
    gInputHLE.Poll();
    ExtendedController& ctrl = gInputHLE.GetController(0);

    inputs[0] = 0;
    if(ctrl.GetButton(Button::A))      inputs[0] |= 0x0001;
    if(ctrl.GetButton(Button::B))      inputs[0] |= 0x0002;
    if(ctrl.GetButton(Button::X))      inputs[0] |= 0x0004;
    if(ctrl.GetButton(Button::Y))      inputs[0] |= 0x0008;
    if(ctrl.GetButton(Button::L1))     inputs[0] |= 0x0010;
    if(ctrl.GetButton(Button::R1))     inputs[0] |= 0x0020;
    if(ctrl.GetButton(Button::L2))     inputs[0] |= 0x0040;
    if(ctrl.GetButton(Button::R2))     inputs[0] |= 0x0080;
    if(ctrl.GetButton(Button::Start))  inputs[0] |= 0x0100;
    if(ctrl.GetButton(Button::Select)) inputs[0] |= 0x0200;
    if(ctrl.GetButton(Button::Up))     inputs[0] |= 0x0400;
    if(ctrl.GetButton(Button::Down))   inputs[0] |= 0x0800;
    if(ctrl.GetButton(Button::Left))   inputs[0] |= 0x1000;
    if(ctrl.GetButton(Button::Right))  inputs[0] |= 0x2000;
    if(ctrl.GetButton(Button::LS))     inputs[0] |= 0x4000;
    if(ctrl.GetButton(Button::RS))     inputs[0] |= 0x8000;

    float lx, ly, rx, ry;
    ctrl.GetLeftStick(lx, ly);
    ctrl.GetRightStick(rx, ry);

    auto StickToWord = [](float f)->WORD { return WORD((std::clamp(f, -1.0f,1.0f)+1.0f)*0.5f*65535.0f); };
    r->GPR[A0*2] = StickToWord(lx);
    r->GPR[A1*2] = StickToWord(ly);
    r->GPR[A2*2] = StickToWord(rx);
    r->GPR[A3*2] = StickToWord(ry);

    float gx, gy, gz, ax, ay, az;
    ctrl.GetGyro(gx, gy, gz);
    ctrl.GetAccel(ax, ay, az);
    r->FPR[0] = gx; r->FPR[1] = gy; r->FPR[2] = gz;
    r->FPR[3] = ax; r->FPR[4] = ay; r->FPR[5] = az;
}

// -------------------------- HLE Timer --------------------------
HLETimer gTimer;

// -------------------------- Audio --------------------------
AudioHLE gAudioHLE(44100,16);

// -------------------------- GPU / Framebuffer --------------------------
struct Color { uint8_t r,g,b,a; };
std::vector<Color> framebuffer;
int fbWidth=640, fbHeight=480;
std::mutex gpuMutex;

void GPUInit(int width=640,int height=480) {
    std::lock_guard<std::mutex> lock(gpuMutex);
    fbWidth = width; fbHeight = height;
    framebuffer.resize(width*height, Color{0,0,0,255});
}

void GPUClear(const Color& c) {
    std::lock_guard<std::mutex> lock(gpuMutex);
    std::fill(framebuffer.begin(), framebuffer.end(), c);
}

// Simplified triangle rasterization (flat)
void GPUDrawTriangle(const Vertex& v0,const Vertex& v1,const Vertex& v2) {
    std::lock_guard<std::mutex> lock(gpuMutex);
    int minX = std::max(0,int(std::floor(std::min({v0.x,v1.x,v2.x}))));
    int maxX = std::min(fbWidth-1,int(std::ceil(std::max({v0.x,v1.x,v2.x}))));
    int minY = std::max(0,int(std::floor(std::min({v0.y,v1.y,v2.y}))));
    int maxY = std::min(fbHeight-1,int(std::ceil(std::max({v0.y,v1.y,v2.y}))));

    auto EdgeFunc=[](const Vertex& a,const Vertex& b,float x,float y){ return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x); };

    for(int y=minY;y<=maxY;y++)
        for(int x=minX;x<=maxX;x++){
            float w0=EdgeFunc(v1,v2,float(x)+0.5f,float(y)+0.5f);
            float w1=EdgeFunc(v2,v0,float(x)+0.5f,float(y)+0.5f);
            float w2=EdgeFunc(v0,v1,float(x)+0.5f,float(y)+0.5f);
            if(w0>=0 && w1>=0 && w2>=0){
                int idx=y*fbWidth+x;
                framebuffer[idx] = Color{
                    (v0.color.r+v1.color.r+v2.color.r)/3,
                    (v0.color.g+v1.color.g+v2.color.g)/3,
                    (v0.color.b+v1.color.b+v2.color.b)/3,
                    255
                };
            }
        }
}

// -------------------------- Main Loop --------------------------
void HLERun() {
    GPUInit(640,480);
    gAudioHLE.Start();
    gTimer.Start(60, [](){ hleKeyInput(); }); // 60Hz HLE input poll

    while(appletMainLoop()) {
        // Input + Timer handled by HLETimer & hleKeyInput
        // Render placeholder
        GPUClear(Color{0,0,0,255});

        // Example triangle
        Vertex v0{320,100,0,Color{255,0,0,255}};
        Vertex v1{100,380,0,Color{0,255,0,255}};
        Vertex v2{540,380,0,Color{0,0,255,255}};
        GPUDrawTriangle(v0,v1,v2);

        // Simulate audio update
        gAudioHLE.GetMixedBuffer(512);

        // Display framebuffer via libnx (simplified)
        // In practice, copy framebuffer to NX graphics buffer
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
    }

    gTimer.Stop();
    gAudioHLE.Stop();
}
