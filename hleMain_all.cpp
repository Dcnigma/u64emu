#include <vector>
#include "math.h"
#include "ki.h"

#include "hleMain.h"
#include "iMemory.h"
#include "dynaNative.h"
#include "dynaFP.h"
#include "hleDSP.h"
#include "iCPU.h"



#include <unordered_map>
#include <functional>
#include <cmath>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <queue>

#include <switch/svc.h>
#include <switch/ndsp.h>
#include <switch.h>

#define SHOW_NAME(a)

extern WORD inputs[4];
bool hleFMVDelay = false;

void hleFMVSub2Patch();
void hleKeyInput();

// Identity matrices
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

// Timing utilities
std::atomic<bool> g_running(true);
uint64_t g_ticksStart = 0;

inline uint64_t GetTicks()
{
    armGetSystemTick(); // libnx ARM64 system tick
}

inline double TicksToSeconds(uint64_t ticks)
{
    return static_cast<double>(ticks) / 192'000'000.0;
}

// Initialization
bool HLE_Init()
{
    romfsInit();       // Initialize ROM filesystem
    fsInit();          // libnx FS init
    g_ticksStart = GetTicks();
    return true;
}

void HLE_Shutdown()
{
    romfsExit();
    fsExit();
}

void hleICache()
{
    DWORD addr = r->GPR[A0*2];
    DWORD len = r->GPR[A1*2];
    dynaInvalidate(addr, len);
}

void hleVirtualToPhysical()
{
    DWORD addr = r->GPR[A0*2];
    if(addr < 0x00400000)
        addr |= 0x80000000;
    else if(addr < 0x05000000)
        addr |= 0xa0000000;
    else
        addr &= 0x3fffff;

    r->GPR[V0*2] = addr;
}

void hlePiRawStartDma() { /* stub */ }
void hlePiStartDma()    { /* stub */ }

void hleCos()
{
    double result = cos((float)*(float*)&r->FPR[12]);
    *(float*)&r->FPR[0] = (float)result;
}

void hleSin()
{
    double result = sin((float)*(float*)&r->FPR[12]);
    *(float*)&r->FPR[0] = (float)result;
}

void hleNormalize(float* vec)
{
    float x = vec[0];
    float y = vec[1];
    float z = vec[2];

    float len = sqrtf(x*x + y*y + z*z);
    if(len != 0.f)
    {
        vec[0] = x / len;
        vec[1] = y / len;
        vec[2] = z / len;
    }
}

void hleMtxIdentF(float* m) { for(int i=0;i<16;i++) m[i] = ident[i]; }
void hleMtxIdent(WORD* m) { for(int i=0;i<32;i++) m[i] = identl[i]; }
void hleMtxCopyF(float* dst, float* src) { for(int i=0;i<16;i++) dst[i] = src[i]; }
void hleMtxCopy(WORD* dst, WORD* src) { for(int i=0;i<32;i++) dst[i] = src[i]; }

void hleVecCross(float* dst, float* a, float* b)
{
    dst[0] = a[1]*b[2] - a[2]*b[1];
    dst[1] = a[2]*b[0] - a[0]*b[2];
    dst[2] = a[0]*b[1] - a[1]*b[0];
}

float hleVecDot(float* a, float* b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void hleVecScale(float* dst, float* v, float s)
{
    dst[0] = v[0]*s;
    dst[1] = v[1]*s;
    dst[2] = v[2]*s;
}

void hleMemSet(void* dst, int val, size_t size)
{
    memset(dst, val, size);
}

void hleMemCpy(void* dst, const void* src, size_t size)
{
    memcpy(dst, src, size);
}

void hleDmaCopy(void* dst, const void* src, size_t size)
{
    hleMemCpy(dst, src, size);
}

void* hleAlloc(size_t size) { return malloc(size); }
void hleFree(void* ptr) { if(ptr) free(ptr); }
// ------------------------ Audio (NDSP) ------------------------

const int HLE_AUDIO_SAMPLE_RATE = 44100;
const int HLE_AUDIO_BUFFER_SIZE = 4096;

float hleAudioBuffer[HLE_AUDIO_BUFFER_SIZE];
std::atomic<size_t> hleAudioWritePos(0);

std::mutex hleAudioMutex;

struct HLEAudioChannel {
    bool active = false;
    float frequency = 440.0f;
    float amplitude = 0.5f;
    float phase = 0.0f;
    enum Waveform { SINE, SQUARE, TRIANGLE, SAW } waveform = SINE;

    void mix(float* buffer, size_t samples) {
        if(!active) return;
        float phaseInc = 2.f * 3.14159265f * frequency / HLE_AUDIO_SAMPLE_RATE;
        for(size_t i = 0; i < samples; i++) {
            float sample = 0.f;
            switch(waveform) {
                case SINE: sample = std::sin(phase); break;
                case SQUARE: sample = (phase < 3.14159265f) ? 1.f : -1.f; break;
                case TRIANGLE: sample = 2.f * std::abs(2.f*(phase/(2.f*3.14159265f) - std::floor(phase/(2.f*3.14159265f)+0.5f))) - 1.f; break;
                case SAW: sample = 2.f*(phase/(2.f*3.14159265f) - std::floor(phase/(2.f*3.14159265f)+0.5f)); break;
            }
            buffer[i] += sample * amplitude;
            phase += phaseInc;
            if(phase > 2.f * 3.14159265f) phase -= 2.f * 3.14159265f;
        }
    }
};

HLEAudioChannel hleChannels[16];

// Initialize NDSP audio
void AXInit() {
    std::lock_guard<std::mutex> lock(hleAudioMutex);
    ndspInit();
    for(auto& ch : hleChannels) ch.active = false;
    memset(hleAudioBuffer, 0, sizeof(hleAudioBuffer));
}

// Play tone
void AXPlayTone(int channel, float freq, float amplitude=0.5f, HLEAudioChannel::Waveform wave=HLEAudioChannel::SINE) {
    if(channel < 0 || channel >= 16) return;
    hleChannels[channel].frequency = freq;
    hleChannels[channel].amplitude = amplitude;
    hleChannels[channel].waveform = wave;
    hleChannels[channel].phase = 0.f;
    hleChannels[channel].active = true;
}

// Stop tone
void AXStop(int channel) {
    if(channel < 0 || channel >= 16) return;
    hleChannels[channel].active = false;
}

// Mix all channels into buffer
void AXMix(float* outBuffer, size_t samples) {
    std::lock_guard<std::mutex> lock(hleAudioMutex);
    memset(outBuffer, 0, samples*sizeof(float));
    for(int i=0;i<16;i++)
        hleChannels[i].mix(outBuffer, samples);
}

// Update NDSP buffer
void AXUpdateBuffer() {
    std::lock_guard<std::mutex> lock(hleAudioMutex);

    // Prepare a stereo buffer
    static float ndspBuf[HLE_AUDIO_BUFFER_SIZE*2];
    AXMix(hleAudioBuffer, HLE_AUDIO_BUFFER_SIZE);

    for(size_t i=0;i<HLE_AUDIO_BUFFER_SIZE;i++){
        ndspBuf[i*2] = hleAudioBuffer[i];
        ndspBuf[i*2+1] = hleAudioBuffer[i];
    }

    ndspWaveBuf wave;
    memset(&wave, 0, sizeof(wave));
    wave.data_vaddr = ndspBuf;
    wave.nsamples = HLE_AUDIO_BUFFER_SIZE;
    wave.looping = false;
    wave.status = NDSP_WBUF_DONE;
    ndspChnWaveBufAdd(0, &wave);  // Channel 0
}

// ------------------------ Input (Joy-Con) ------------------------

VirtualController gControllers[2];

void PollInput() {
    hidScanInput();
    u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
    u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

    auto& ctrl = gControllers[0];

    ctrl.SetButton(Button::A, kHeld & KEY_A);
    ctrl.SetButton(Button::B, kHeld & KEY_B);
    ctrl.SetButton(Button::X, kHeld & KEY_X);
    ctrl.SetButton(Button::Y, kHeld & KEY_Y);
    ctrl.SetButton(Button::L1, kHeld & KEY_L);
    ctrl.SetButton(Button::R1, kHeld & KEY_R);
    ctrl.SetButton(Button::L2, kHeld & KEY_ZL);
    ctrl.SetButton(Button::R2, kHeld & KEY_ZR);
    ctrl.SetButton(Button::Start, kHeld & KEY_PLUS);
    ctrl.SetButton(Button::Select, kHeld & KEY_MINUS);
    ctrl.SetButton(Button::Up, kHeld & KEY_DUP);
    ctrl.SetButton(Button::Down, kHeld & KEY_DDOWN);
    ctrl.SetButton(Button::Left, kHeld & KEY_DLEFT);
    ctrl.SetButton(Button::Right, kHeld & KEY_DRIGHT);
}

// ------------------------ GPU / Framebuffer ------------------------

struct Color { uint8_t r,g,b,a; };
struct Vertex { float x,y,z; Color color; };

std::vector<Color> framebuffer;
int fbWidth = 640;
int fbHeight = 480;
std::mutex gpuMutex;

void GPUInit(int width=640,int height=480){
    std::lock_guard<std::mutex> lock(gpuMutex);
    fbWidth=width; fbHeight=height;
    framebuffer.resize(width*height, {0,0,0,255});
}

void GPUClear(const Color& c){
    std::lock_guard<std::mutex> lock(gpuMutex);
    std::fill(framebuffer.begin(), framebuffer.end(), c);
}

void GPUDrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2){
    std::lock_guard<std::mutex> lock(gpuMutex);
    float minX=std::min({v0.x,v1.x,v2.x});
    float maxX=std::max({v0.x,v1.x,v2.x});
    float minY=std::min({v0.y,v1.y,v2.y});
    float maxY=std::max({v0.y,v1.y,v2.y});

    int x0=std::max(int(minX),0), x1=std::min(int(maxX), fbWidth-1);
    int y0=std::max(int(minY),0), y1=std::min(int(maxY), fbHeight-1);

    auto Edge=[=](const Vertex& a, const Vertex& b, float x, float y){ return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x); };

    for(int y=y0;y<=y1;y++){
        for(int x=x0;x<=x1;x++){
            float w0=Edge(v1,v2,float(x)+0.5f,float(y)+0.5f);
            float w1=Edge(v2,v0,float(x)+0.5f,float(y)+0.5f);
            float w2=Edge(v0,v1,float(x)+0.5f,float(y)+0.5f);
            if(w0>=0 && w1>=0 && w2>=0){
                int idx=y*fbWidth+x;
                framebuffer[idx]={
                    uint8_t((v0.color.r+v1.color.r+v2.color.r)/3),
                    uint8_t((v0.color.g+v1.color.g+v2.color.g)/3),
                    uint8_t((v0.color.b+v1.color.b+v2.color.b)/3),
                    255
                };
            }
        }
    }
}

const std::vector<Color>& GPUGetFramebuffer(){ return framebuffer; }


// ------------------------ HLE Timer ------------------------

class HLETimer {
public:
    HLETimer() : running(false), tickCount(0) {}

    void Start(int fps, std::function<void()> tickCallback) {
        std::lock_guard<std::mutex> lock(mutex);
        if(running) return;

        running = true;
        tickThread = std::thread([=]() {
            auto tickDuration = std::chrono::milliseconds(1000 / fps);
            while(running) {
                auto start = std::chrono::steady_clock::now();
                tickCallback();
                tickCount++;
                auto end = std::chrono::steady_clock::now();
                auto elapsed = end - start;
                if(elapsed < tickDuration)
                    std::this_thread::sleep_for(tickDuration - elapsed);
            }
        });
    }

    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            running = false;
        }
        if(tickThread.joinable())
            tickThread.join();
    }

    uint64_t GetTicks() {
        return tickCount;
    }

    ~HLETimer() {
        Stop();
    }

private:
    std::atomic<bool> running;
    std::atomic<uint64_t> tickCount;
    std::thread tickThread;
    std::mutex mutex;
};

// ------------------------ HLE Sound (Console/NDSP) ------------------------

class HLESound {
public:
    HLESound() : running(false) {}

    void PlayTone(double frequency, int durationMs) {
        std::lock_guard<std::mutex> lock(mutex);
        soundQueue.push([=](){ AXPlayTone(0, float(frequency)); });
        // Auto-stop after duration
        soundQueue.push([=](){
            std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
            AXStop(0);
        });
    }

    void PlaySample(std::function<void()> sampleFunc) {
        std::lock_guard<std::mutex> lock(mutex);
        soundQueue.push(sampleFunc);
    }

    void Start() {
        std::lock_guard<std::mutex> lock(mutex);
        if(running) return;
        running = true;
        audioThread = std::thread([this]() {
            while(running) {
                std::function<void()> task;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if(!soundQueue.empty()) {
                        task = soundQueue.front();
                        soundQueue.pop();
                    }
                }
                if(task) task();
                else std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            running = false;
        }
        if(audioThread.joinable())
            audioThread.join();
    }

    ~HLESound() { Stop(); }

private:
    std::atomic<bool> running;
    std::thread audioThread;
    std::mutex mutex;
    std::queue<std::function<void()>> soundQueue;
};

// ------------------------ HLE Main Loop ------------------------

class HLECore {
public:
    HLECore() : fps(60), running(false) {}

    void Init() {
        AXInit();
        GPUInit();
        timer.Start(fps, [this](){ this->Tick(); });
        sound.Start();
    }

    void Tick() {
        PollInput();           // Poll Joy-Con/keyboard input
        AXUpdateBuffer();      // Update audio
        // TODO: Insert your emulated CPU tick / HLE CPU functions here
        // e.g. hleMainStep();
    }

    void Run() {
        running = true;
        while(running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // This loop could integrate frame drawing to libnx screen later
        }
    }

    void Stop() {
        running = false;
        timer.Stop();
        sound.Stop();
    }

private:
    int fps;
    std::atomic<bool> running;
    HLETimer timer;
    HLESound sound;
};

HLECore gHLECore;

// ------------------------ Framebuffer ------------------------

struct Color {
    uint8_t r, g, b, a;
    Color(uint8_t _r=0, uint8_t _g=0, uint8_t _b=0, uint8_t _a=255)
        : r(_r), g(_g), b(_b), a(_a) {}
};

std::vector<Color> framebuffer;
int fbWidth  = 1280; // Default Switch top screen width
int fbHeight = 720;  // Default Switch top screen height
std::mutex fbMutex;

// ------------------------ GPU Init ------------------------

void GPUInit(int width=1280, int height=720) {
    std::lock_guard<std::mutex> lock(fbMutex);
    fbWidth = width;
    fbHeight = height;
    framebuffer.resize(fbWidth * fbHeight, Color(0,0,0,255));
}

// Clear framebuffer with color
void GPUClear(const Color& c) {
    std::lock_guard<std::mutex> lock(fbMutex);
    std::fill(framebuffer.begin(), framebuffer.end(), c);
}

// Draw pixel
void GPUSetPixel(int x, int y, const Color& c) {
    if(x<0 || x>=fbWidth || y<0 || y>=fbHeight) return;
    std::lock_guard<std::mutex> lock(fbMutex);
    framebuffer[y * fbWidth + x] = c;
}

// ------------------------ Framebuffer Swap to Switch ------------------------

void GPUFlushToScreen() {
    std::lock_guard<std::mutex> lock(fbMutex);

    Framebuffer fb;
    Result rc = framebufCreate(&fb, fbWidth, fbHeight, PIXEL_FORMAT_RGBA_8888, 2);
    if(R_FAILED(rc)) return;

    uint32_t* fbPtr = (uint32_t*)fb.addr[0];
    for(int y=0; y<fbHeight; y++) {
        for(int x=0; x<fbWidth; x++) {
            const Color& c = framebuffer[y*fbWidth + x];
            fbPtr[y*fbWidth + x] = (c.a<<24)|(c.b<<16)|(c.g<<8)|c.r;
        }
    }

    framebufFlush(&fb);
    framebufFree(&fb);
}

// ------------------------ GPU Draw Triangle (flat) ------------------------

struct Vertex {
    float x, y, z;
    Color color;
};

static float EdgeFunction(const Vertex& a, const Vertex& b, float px, float py) {
    return (px - a.x)*(b.y - a.y) - (py - a.y)*(b.x - a.x);
}

void GPUDrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    int minX = std::max(0, int(std::floor(std::min({v0.x, v1.x, v2.x}))));
    int maxX = std::min(fbWidth-1, int(std::ceil(std::max({v0.x, v1.x, v2.x}))));
    int minY = std::max(0, int(std::floor(std::min({v0.y, v1.y, v2.y}))));
    int maxY = std::min(fbHeight-1, int(std::ceil(std::max({v0.y, v1.y, v2.y}))));

    float area = EdgeFunction(v0, v1, v2.x, v2.y);
    if(std::abs(area) < 1e-6f) return; // degenerate triangle

    for(int y=minY; y<=maxY; y++) {
        for(int x=minX; x<=maxX; x++) {
            float w0 = EdgeFunction(v1, v2, x+0.5f, y+0.5f);
            float w1 = EdgeFunction(v2, v0, x+0.5f, y+0.5f);
            float w2 = EdgeFunction(v0, v1, x+0.5f, y+0.5f);
            if(w0>=0 && w1>=0 && w2>=0) {
                float sum = w0 + w1 + w2;
                uint8_t r = uint8_t((v0.color.r*w0 + v1.color.r*w1 + v2.color.r*w2)/sum);
                uint8_t g = uint8_t((v0.color.g*w0 + v1.color.g*w1 + v2.color.g*w2)/sum);
                uint8_t b = uint8_t((v0.color.b*w0 + v1.color.b*w1 + v2.color.b*w2)/sum);
                GPUSetPixel(x, y, Color(r,g,b,255));
            }
        }
    }
}

// ------------------------ GPU Draw Textured Triangle ------------------------

struct Texture {
    int width, height;
    std::vector<Color> data;

    Texture(int w=0, int h=0) : width(w), height(h), data(w*h) {}

    Color Sample(float u, float v) const {
        int tx = std::clamp(int(u * width), 0, width-1);
        int ty = std::clamp(int(v * height), 0, height-1);
        return data[ty * width + tx];
    }
};

void GPUDrawTexturedTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Texture& tex) {
    int minX = std::max(0, int(std::floor(std::min({v0.x, v1.x, v2.x}))));
    int maxX = std::min(fbWidth-1, int(std::ceil(std::max({v0.x, v1.x, v2.x}))));
    int minY = std::max(0, int(std::floor(std::min({v0.y, v1.y, v2.y}))));
    int maxY = std::min(fbHeight-1, int(std::ceil(std::max({v0.y, v1.y, v2.y}))));

    auto Edge = [](const Vertex& a, const Vertex& b, float px, float py) {
        return (px - a.x)*(b.y - a.y) - (py - a.y)*(b.x - a.x);
    };

    float area = Edge(v0, v1, v2.x, v2.y);
    if(std::abs(area) < 1e-6f) return;

    for(int y=minY; y<=maxY; y++) {
        for(int x=minX; x<=maxX; x++) {
            float w0 = Edge(v1, v2, x+0.5f, y+0.5f);
            float w1 = Edge(v2, v0, x+0.5f, y+0.5f);
            float w2 = Edge(v0, v1, x+0.5f, y+0.5f);
            if(w0>=0 && w1>=0 && w2>=0) {
                float sum = w0 + w1 + w2;
                float u = (v0.u*w0 + v1.u*w1 + v2.u*w2)/sum;
                float v = (v0.v*w0 + v1.v*w1 + v2.v*w2)/sum;
                GPUSetPixel(x, y, tex.Sample(u,v));
            }
        }
    }
}

// ------------------------ GPU Framebuffer Access ------------------------

const std::vector<Color>& GPUGetFramebuffer() {
    return framebuffer;
}



// Include your previously defined GPU, Audio, and Input systems here
// GPUInit, GPUClear, GPUDrawTriangle, GPUFlushToScreen
// AudioHLE, VirtualController, InputHLE

// ------------------------ HLE Main Loop ------------------------

class HLESystem {
public:
    HLESystem(int width=1280, int height=720, int fps=60)
        : fbWidth(width), fbHeight(height), targetFPS(fps), running(false) {}

    void Init() {
        // Initialize GPU framebuffer
        GPUInit(fbWidth, fbHeight);
        GPUClear(Color(0,0,0,255));

        // Initialize audio
        audio.Start();

        // Initialize input
        hidInitialize();

        std::cout << "HLESystem initialized.\n";
    }

    void Run() {
        running = true;
        mainLoopThread = std::thread([this]() { this->MainLoop(); });
    }

    void Stop() {
        running = false;
        if(mainLoopThread.joinable())
            mainLoopThread.join();

        audio.Stop();
        hidExit();
        std::cout << "HLESystem stopped.\n";
    }

    ~HLESystem() {
        Stop();
    }

    // Access virtual controllers
    InputHLE& GetInput() { return input; }
    AudioHLE& GetAudio() { return audio; }

private:
    int fbWidth, fbHeight;
    int targetFPS;
    std::atomic<bool> running;
    std::thread mainLoopThread;

    InputHLE input{2};      // Two virtual controllers
    AudioHLE audio{44100, 16};  // 16 channels

    void MainLoop() {
        const std::chrono::milliseconds frameDuration(1000 / targetFPS);

        while(running) {
            auto frameStart = std::chrono::steady_clock::now();

            // -------------------- Poll Input --------------------
            PadState pad;
            padUpdate(&pad);
            for(int i=0; i<2; i++) {
                PadController pc;
                padGetButtons(&pc, i); // libnx mapping
                VirtualController& vc = input.GetController(i);

                vc.SetButton(Button::A, pc.button & HidNpadButton_A);
                vc.SetButton(Button::B, pc.button & HidNpadButton_B);
                vc.SetButton(Button::X, pc.button & HidNpadButton_X);
                vc.SetButton(Button::Y, pc.button & HidNpadButton_Y);
                vc.SetButton(Button::L1, pc.button & HidNpadButton_L);
                vc.SetButton(Button::R1, pc.button & HidNpadButton_R);
                vc.SetButton(Button::L2, pc.button & HidNpadButton_ZL);
                vc.SetButton(Button::R2, pc.button & HidNpadButton_ZR);
                vc.SetButton(Button::Start, pc.button & HidNpadButton_Plus);
                vc.SetButton(Button::Select, pc.button & HidNpadButton_Minus);
                vc.SetButton(Button::Up, pc.button & HidNpadButton_Up);
                vc.SetButton(Button::Down, pc.button & HidNpadButton_Down);
                vc.SetButton(Button::Left, pc.button & HidNpadButton_Left);
                vc.SetButton(Button::Right, pc.button & HidNpadButton_Right);
            }

            // -------------------- Run HLE Tick --------------------
            HLETick();

            // -------------------- Render GPU --------------------
            GPUFlushToScreen();

            // -------------------- Sleep to maintain FPS --------------------
            auto frameEnd = std::chrono::steady_clock::now();
            auto elapsed = frameEnd - frameStart;
            if(elapsed < frameDuration) {
                std::this_thread::sleep_for(frameDuration - elapsed);
            }
        }
    }

    // -------------------- HLE Tick --------------------
    void HLETick() {
        // Example: clear screen every tick (replace with real emulation logic)
        GPUClear(Color(0,0,64,255));

        // Example: draw moving triangle (demo)
        static float t = 0;
        t += 1.0f;
        Vertex v0{100 + 200*std::sin(t*0.01f), 100, 0, Color(255,0,0)};
        Vertex v1{400, 100 + 200*std::cos(t*0.012f), 0, Color(0,255,0)};
        Vertex v2{250, 400, 0, Color(0,0,255)};
        GPUDrawTriangle(v0,v1,v2);

        // Example audio tick: play sine tone on channel 0
        audio.PlaySample(nullptr); // Replace with real sample / mixer
    }
};


// Include previous GPU, Audio, Input, Timer systems
// Also include your previously ported hleMain.cpp math, memory, DMA, NAND, FMV functions

// -------------------- Global HLE State --------------------

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

// -------------------- Memory / DMA --------------------
void hleMemSet(void* dst, int val, size_t size) {
    std::memset(dst, val, size);
}

void hleMemCpy(void* dst, const void* src, size_t size) {
    std::memcpy(dst, src, size);
}

void hleDmaCopy(void* dst, const void* src, size_t size) {
    hleMemCpy(dst, src, size);
}

void* hleAlloc(size_t size) { return malloc(size); }
void hleFree(void* ptr) { if(ptr) free(ptr); }

// -------------------- Math / Vector / Matrix --------------------
void hleNormalize(float* vec) {
    float len = std::sqrt(vec[0]*vec[0]+vec[1]*vec[1]+vec[2]*vec[2]);
    if(len != 0.f) {
        vec[0]/=len; vec[1]/=len; vec[2]/=len;
    }
}

void hleMtxIdentF(float* m) {
    std::memcpy(m, ident, sizeof(float)*16);
}

void hleMtxIdent(WORD* m) {
    std::memcpy(m, identl, sizeof(WORD)*32);
}

void hleMtxCopyF(float* dst, float* src) { std::memcpy(dst, src, sizeof(float)*16); }
void hleMtxCopy(WORD* dst, WORD* src) { std::memcpy(dst, src, sizeof(WORD)*32); }

void hleVecCross(float* dst, float* a, float* b) {
    dst[0] = a[1]*b[2]-a[2]*b[1];
    dst[1] = a[2]*b[0]-a[0]*b[2];
    dst[2] = a[0]*b[1]-a[1]*b[0];
}

float hleVecDot(float* a, float* b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
void hleVecScale(float* dst, float* v, float s) { dst[0]=v[0]*s; dst[1]=v[1]*s; dst[2]=v[2]*s; }

void hleCos() { *(float*)&r->FPR[0] = cosf(*(float*)&r->FPR[12]); }
void hleSin() { *(float*)&r->FPR[0] = sinf(*(float*)&r->FPR[12]); }

// -------------------- FMV / Key Input --------------------
void hleFMVSub2Patch() { /* your FMV patch logic */ }
void hleKeyInput() { /* poll input / controller logic */ }

// -------------------- NAND / File System --------------------
namespace fs = std::filesystem;
std::mutex fsMutex;
std::string nandRoot = "./hle_nand/";

void FSInit(const std::string& path = "./hle_nand/") {
    std::lock_guard<std::mutex> lock(fsMutex);
    nandRoot = path;
    if(!fs::exists(nandRoot)) fs::create_directories(nandRoot);
}

bool FSSaveFile(const std::string& fileName, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(fsMutex);
    std::ofstream file(nandRoot+fileName, std::ios::binary);
    if(!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

bool FSLoadFile(const std::string& fileName, std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(fsMutex);
    std::ifstream file(nandRoot+fileName, std::ios::binary|std::ios::ate);
    if(!file.is_open()) return false;
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return true;
}

bool FSFileExists(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(fsMutex);
    return fs::exists(nandRoot + fileName);
}

bool FSDeleteFile(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(fsMutex);
    return fs::remove(nandRoot + fileName);
}

std::vector<std::string> FSListFiles(const std::string& directory = "") {
    std::lock_guard<std::mutex> lock(fsMutex);
    std::vector<std::string> files;
    std::string fullPath = nandRoot + directory;
    if(!fs::exists(fullPath)) return files;
    for(auto& entry : fs::directory_iterator(fullPath))
        if(entry.is_regular_file()) files.push_back(entry.path().filename().string());
    return files;
}

// -------------------- HLE Main System --------------------
class HLESwitch {
public:
    HLESwitch(int w=1280, int h=720, int fps=60)
        : fbWidth(w), fbHeight(h), targetFPS(fps), running(false) {}

    void Init() {
        GPUInit(fbWidth, fbHeight);
        GPUClear(Color(0,0,0,255));
        audio.Start();
        hidInitialize();
        FSInit();
        std::cout << "HLESwitch initialized.\n";
    }

    void Run() {
        running = true;
        mainLoopThread = std::thread([this](){ this->MainLoop(); });
    }

    void Stop() {
        running = false;
        if(mainLoopThread.joinable()) mainLoopThread.join();
        audio.Stop();
        hidExit();
        std::cout << "HLESwitch stopped.\n";
    }

    InputHLE& GetInput() { return input; }
    AudioHLE& GetAudio() { return audio; }

private:
    int fbWidth, fbHeight, targetFPS;
    std::atomic<bool> running;
    std::thread mainLoopThread;
    InputHLE input{2};
    AudioHLE audio{44100,16};

    void MainLoop() {
        const std::chrono::milliseconds frameDuration(1000/targetFPS);

        while(running) {
            auto frameStart = std::chrono::steady_clock::now();

            // -------------------- Poll Input --------------------
            padUpdate(&pad);
            for(int i=0;i<2;i++){
                PadController pc;
                padGetButtons(&pc,i);
                VirtualController& vc = input.GetController(i);
                vc.SetButton(Button::A, pc.button & HidNpadButton_A);
                vc.SetButton(Button::B, pc.button & HidNpadButton_B);
                vc.SetButton(Button::X, pc.button & HidNpadButton_X);
                vc.SetButton(Button::Y, pc.button & HidNpadButton_Y);
                vc.SetButton(Button::L1, pc.button & HidNpadButton_L);
                vc.SetButton(Button::R1, pc.button & HidNpadButton_R);
                vc.SetButton(Button::L2, pc.button & HidNpadButton_ZL);
                vc.SetButton(Button::R2, pc.button & HidNpadButton_ZR);
                vc.SetButton(Button::Start, pc.button & HidNpadButton_Plus);
                vc.SetButton(Button::Select, pc.button & HidNpadButton_Minus);
                vc.SetButton(Button::Up, pc.button & HidNpadButton_Up);
                vc.SetButton(Button::Down, pc.button & HidNpadButton_Down);
                vc.SetButton(Button::Left, pc.button & HidNpadButton_Left);
                vc.SetButton(Button::Right, pc.button & HidNpadButton_Right);
            }

            // -------------------- HLE Tick --------------------
            HLETick();

            // -------------------- Render GPU --------------------
            GPUFlushToScreen();

            auto frameEnd = std::chrono::steady_clock::now();
            auto elapsed = frameEnd - frameStart;
            if(elapsed < frameDuration) std::this_thread::sleep_for(frameDuration - elapsed);
        }
    }

    void HLETick() {
        // Example: clear screen each tick
        GPUClear(Color(0,0,64,255));

        // Example FMV / HLE logic
        hleFMVSub2Patch();
        hleKeyInput();

        // Example: draw demo triangle
        static float t=0; t+=1.0f;
        Vertex v0{100 + 200*std::sin(t*0.01f), 100,0, Color(255,0,0)};
        Vertex v1{400,100 + 200*std::cos(t*0.012f),0, Color(0,255,0)};
        Vertex v2{250,400,0, Color(0,0,255)};
        GPUDrawTriangle(v0,v1,v2);

        // Audio tick placeholder
        audio.PlaySample(nullptr);
    }

    PadState pad;
};


// -------------------- Color / Vertex --------------------
struct Color {
    uint8_t r, g, b, a;
    Color(uint8_t _r=0, uint8_t _g=0, uint8_t _b=0, uint8_t _a=255) : r(_r), g(_g), b(_b), a(_a) {}
};

struct Vertex {
    float x, y, z;
    Color color;
};

// -------------------- GPU libnx Rendering --------------------
class GPU {
public:
    GPU(int w=1280, int h=720) : width(w), height(h), frameBuffer(nullptr) {}

    void Init() {
        consoleInit(NULL);
        frameBuffer = new Color[width*height];
        Clear(Color(0,0,0,255));
    }

    void Clear(const Color& c) {
        std::lock_guard<std::mutex> lock(mutex);
        for(int i=0;i<width*height;i++) frameBuffer[i] = c;
    }

    void DrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
        std::lock_guard<std::mutex> lock(mutex);

        // Bounding box
        int minX = std::max(0, (int)std::floor(std::min({v0.x,v1.x,v2.x})));
        int maxX = std::min(width-1, (int)std::ceil(std::max({v0.x,v1.x,v2.x})));
        int minY = std::max(0, (int)std::floor(std::min({v0.y,v1.y,v2.y})));
        int maxY = std::min(height-1, (int)std::ceil(std::max({v0.y,v1.y,v2.y})));

        auto EdgeFunc = [](const Vertex& a, const Vertex& b, float x, float y) {
            return (x-a.x)*(b.y-a.y) - (y-a.y)*(b.x-a.x);
        };

        float denom = EdgeFunc(v0,v1,v2.x,v2.y); // Area check
        if(std::abs(denom)<1e-6f) return;

        for(int y=minY;y<=maxY;y++){
            for(int x=minX;x<=maxX;x++){
                float w0 = EdgeFunc(v1,v2,float(x)+0.5f,float(y)+0.5f)/denom;
                float w1 = EdgeFunc(v2,v0,float(x)+0.5f,float(y)+0.5f)/denom;
                float w2 = 1.0f - w0 - w1;
                if(w0>=0 && w1>=0 && w2>=0){
                    int idx = y*width + x;
                    frameBuffer[idx].r = (uint8_t)(v0.color.r*w0 + v1.color.r*w1 + v2.color.r*w2);
                    frameBuffer[idx].g = (uint8_t)(v0.color.g*w0 + v1.color.g*w1 + v2.color.g*w2);
                    frameBuffer[idx].b = (uint8_t)(v0.color.b*w0 + v1.color.b*w1 + v2.color.b*w2);
                    frameBuffer[idx].a = 255;
                }
            }
        }
    }

    void Present() {
        std::lock_guard<std::mutex> lock(mutex);

        // Map to console framebuffer
        u32* consoleFb = consoleGetFramebuffer(NULL);
        for(int y=0;y<height && y<720;y++){
            for(int x=0;x<width && x<1280;x++){
                int idx = y*width + x;
                u32 col = (frameBuffer[idx].r<<24) | (frameBuffer[idx].g<<16) | (frameBuffer[idx].b<<8) | 255;
                consoleFb[y*1280 + x] = col;
            }
        }
        consoleUpdate(NULL);
    }

    ~GPU() { delete[] frameBuffer; }

private:
    int width, height;
    Color* frameBuffer;
    std::mutex mutex;
};


// -------------------- Color / Vertex --------------------
struct Color {
    uint8_t r, g, b, a;
    Color(uint8_t _r=0, uint8_t _g=0, uint8_t _b=0, uint8_t _a=255) : r(_r), g(_g), b(_b), a(_a) {}
};

struct Vertex {
    float x, y, z;
    Color color;
};

// -------------------- GPU libnx Rendering --------------------
class GPU {
public:
    GPU(int w=1280, int h=720) : width(w), height(h), frameBuffer(nullptr) {}

    void Init() {
        consoleInit(NULL);
        frameBuffer = new Color[width*height];
        Clear(Color(0,0,0,255));
    }

    void Clear(const Color& c) {
        std::lock_guard<std::mutex> lock(mutex);
        for(int i=0;i<width*height;i++) frameBuffer[i] = c;
    }

    void DrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
        std::lock_guard<std::mutex> lock(mutex);

        // Bounding box
        int minX = std::max(0, (int)std::floor(std::min({v0.x,v1.x,v2.x})));
        int maxX = std::min(width-1, (int)std::ceil(std::max({v0.x,v1.x,v2.x})));
        int minY = std::max(0, (int)std::floor(std::min({v0.y,v1.y,v2.y})));
        int maxY = std::min(height-1, (int)std::ceil(std::max({v0.y,v1.y,v2.y})));

        auto EdgeFunc = [](const Vertex& a, const Vertex& b, float x, float y) {
            return (x-a.x)*(b.y-a.y) - (y-a.y)*(b.x-a.x);
        };

        float denom = EdgeFunc(v0,v1,v2.x,v2.y); // Area check
        if(std::abs(denom)<1e-6f) return;

        for(int y=minY;y<=maxY;y++){
            for(int x=minX;x<=maxX;x++){
                float w0 = EdgeFunc(v1,v2,float(x)+0.5f,float(y)+0.5f)/denom;
                float w1 = EdgeFunc(v2,v0,float(x)+0.5f,float(y)+0.5f)/denom;
                float w2 = 1.0f - w0 - w1;
                if(w0>=0 && w1>=0 && w2>=0){
                    int idx = y*width + x;
                    frameBuffer[idx].r = (uint8_t)(v0.color.r*w0 + v1.color.r*w1 + v2.color.r*w2);
                    frameBuffer[idx].g = (uint8_t)(v0.color.g*w0 + v1.color.g*w1 + v2.color.g*w2);
                    frameBuffer[idx].b = (uint8_t)(v0.color.b*w0 + v1.color.b*w1 + v2.color.b*w2);
                    frameBuffer[idx].a = 255;
                }
            }
        }
    }

    void Present() {
        std::lock_guard<std::mutex> lock(mutex);

        // Map to console framebuffer
        u32* consoleFb = consoleGetFramebuffer(NULL);
        for(int y=0;y<height && y<720;y++){
            for(int x=0;x<width && x<1280;x++){
                int idx = y*width + x;
                u32 col = (frameBuffer[idx].r<<24) | (frameBuffer[idx].g<<16) | (frameBuffer[idx].b<<8) | 255;
                consoleFb[y*1280 + x] = col;
            }
        }
        consoleUpdate(NULL);
    }

    ~GPU() { delete[] frameBuffer; }

private:
    int width, height;
    Color* frameBuffer;
    std::mutex mutex;
};


// -------------------- Button Enum --------------------
enum class Button {
    A, B, X, Y,
    L, R, ZL, ZR,
    Start, Select,
    Up, Down, Left, Right,
    LS, RS
};

// -------------------- Virtual Controller --------------------
class VirtualController {
public:
    VirtualController() {
        for(auto b: allButtons)
            states[b] = false;
    }

    void SetButton(Button btn,bool pressed){
        std::lock_guard<std::mutex> lock(mutex);
        states[btn] = pressed;
    }

    bool GetButton(Button btn){
        std::lock_guard<std::mutex> lock(mutex);
        return states[btn];
    }

    void Reset(){
        std::lock_guard<std::mutex> lock(mutex);
        for(auto& kv: states) kv.second = false;
    }

private:
    std::unordered_map<Button,bool> states;
    std::mutex mutex;
    const std::vector<Button> allButtons = {
        Button::A, Button::B, Button::X, Button::Y,
        Button::L, Button::R, Button::ZL, Button::ZR,
        Button::Start, Button::Select,
        Button::Up, Button::Down, Button::Left, Button::Right,
        Button::LS, Button::RS
    };
};

// -------------------- Input HLE --------------------
class InputHLE {
public:
    InputHLE(size_t numControllers=2){
        controllers.resize(numControllers);
        hidInitializeTouchScreen();
        hidInitializeJoy();
    }

    VirtualController& GetController(size_t index){
        if(index >= controllers.size()) index = controllers.size()-1;
        return controllers[index];
    }

    void Poll(){
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
        auto& ctrl = controllers[0];

        ctrl.SetButton(Button::A, kHeld & KEY_A);
        ctrl.SetButton(Button::B, kHeld & KEY_B);
        ctrl.SetButton(Button::X, kHeld & KEY_X);
        ctrl.SetButton(Button::Y, kHeld & KEY_Y);
        ctrl.SetButton(Button::L, kHeld & KEY_L);
        ctrl.SetButton(Button::R, kHeld & KEY_R);
        ctrl.SetButton(Button::ZL, kHeld & KEY_ZL);
        ctrl.SetButton(Button::ZR, kHeld & KEY_ZR);
        ctrl.SetButton(Button::Start, kHeld & KEY_PLUS);
        ctrl.SetButton(Button::Select, kHeld & KEY_MINUS);
        ctrl.SetButton(Button::Up, kHeld & KEY_DUP);
        ctrl.SetButton(Button::Down, kHeld & KEY_DDOWN);
        ctrl.SetButton(Button::Left, kHeld & KEY_DLEFT);
        ctrl.SetButton(Button::Right, kHeld & KEY_DRIGHT);
        ctrl.SetButton(Button::LS, kHeld & KEY_LSTICK);
        ctrl.SetButton(Button::RS, kHeld & KEY_RSTICK);
    }

    ~InputHLE(){
        hidExit();
    }

private:
    std::vector<VirtualController> controllers;
};


// Extend VirtualController to include analog sticks and motion
class ExtendedController : public VirtualController {
public:
    ExtendedController() {
        leftStickX = leftStickY = 0.0f;
        rightStickX = rightStickY = 0.0f;
        gyroX = gyroY = gyroZ = 0.0f;
        accelX = accelY = accelZ = 0.0f;
    }

    void SetLeftStick(float x, float y) {
        std::lock_guard<std::mutex> lock(mutexSticks);
        leftStickX = std::clamp(x, -1.0f, 1.0f);
        leftStickY = std::clamp(y, -1.0f, 1.0f);
    }

    void SetRightStick(float x, float y) {
        std::lock_guard<std::mutex> lock(mutexSticks);
        rightStickX = std::clamp(x, -1.0f, 1.0f);
        rightStickY = std::clamp(y, -1.0f, 1.0f);
    }

    void SetGyro(float x, float y, float z) {
        std::lock_guard<std::mutex> lock(mutexMotion);
        gyroX = x; gyroY = y; gyroZ = z;
    }

    void SetAccel(float x, float y, float z) {
        std::lock_guard<std::mutex> lock(mutexMotion);
        accelX = x; accelY = y; accelZ = z;
    }

    void GetLeftStick(float &x, float &y) {
        std::lock_guard<std::mutex> lock(mutexSticks);
        x = leftStickX; y = leftStickY;
    }

    void GetRightStick(float &x, float &y) {
        std::lock_guard<std::mutex> lock(mutexSticks);
        x = rightStickX; y = rightStickY;
    }

    void GetGyro(float &x, float &y, float &z) {
        std::lock_guard<std::mutex> lock(mutexMotion);
        x = gyroX; y = gyroY; z = gyroZ;
    }

    void GetAccel(float &x, float &y, float &z) {
        std::lock_guard<std::mutex> lock(mutexMotion);
        x = accelX; y = accelY; z = accelZ;
    }

private:
    float leftStickX, leftStickY;
    float rightStickX, rightStickY;
    float gyroX, gyroY, gyroZ;
    float accelX, accelY, accelZ;
    std::mutex mutexSticks;
    std::mutex mutexMotion;
};

// -------------------- InputHLE Extended --------------------
class InputHLE_Extended {
public:
    InputHLE_Extended(size_t numControllers=2) {
        controllers.resize(numControllers);
        hidInitializeJoy();
        hidInitializeAccelerometer();
        hidInitializeGyro();
    }

    ExtendedController& GetController(size_t index){
        if(index >= controllers.size()) index = controllers.size()-1;
        return controllers[index];
    }

    void Poll() {
        hidScanInput();
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
        auto& ctrl = controllers[0];

        // Buttons
        ctrl.SetButton(Button::A, kHeld & KEY_A);
        ctrl.SetButton(Button::B, kHeld & KEY_B);
        ctrl.SetButton(Button::X, kHeld & KEY_X);
        ctrl.SetButton(Button::Y, kHeld & KEY_Y);
        ctrl.SetButton(Button::L, kHeld & KEY_L);
        ctrl.SetButton(Button::R, kHeld & KEY_R);
        ctrl.SetButton(Button::ZL, kHeld & KEY_ZL);
        ctrl.SetButton(Button::ZR, kHeld & KEY_ZR);
        ctrl.SetButton(Button::Start, kHeld & KEY_PLUS);
        ctrl.SetButton(Button::Select, kHeld & KEY_MINUS);
        ctrl.SetButton(Button::Up, kHeld & KEY_DUP);
        ctrl.SetButton(Button::Down, kHeld & KEY_DDOWN);
        ctrl.SetButton(Button::Left, kHeld & KEY_DLEFT);
        ctrl.SetButton(Button::Right, kHeld & KEY_DRIGHT);
        ctrl.SetButton(Button::LS, kHeld & KEY_LSTICK);
        ctrl.SetButton(Button::RS, kHeld & KEY_RSTICK);

        // Analog sticks
        JoystickPosition lStick = hidJoystickRead(CONTROLLER_P1_AUTO, JOYSTICK_LEFT);
        JoystickPosition rStick = hidJoystickRead(CONTROLLER_P1_AUTO, JOYSTICK_RIGHT);
        ctrl.SetLeftStick(lStick.x / 32767.0f, lStick.y / 32767.0f);
        ctrl.SetRightStick(rStick.x / 32767.0f, rStick.y / 32767.0f);

        // Motion sensors
        AccelerometerValues accel = hidAccelRead(CONTROLLER_P1_AUTO);
        GyroscopeValues gyro = hidGyroRead(CONTROLLER_P1_AUTO);
        ctrl.SetAccel(accel.x, accel.y, accel.z);
        ctrl.SetGyro(gyro.x, gyro.y, gyro.z);
    }

    ~InputHLE_Extended() {
        hidExit();
    }

private:
    std::vector<ExtendedController> controllers;
};


// Assume this is your global InputHLE_Extended instance
InputHLE_Extended gInputHLE(2);  // 2 controllers

// Map VirtualController buttons to emulator WORD inputs array
void hleKeyInput() {
    gInputHLE.Poll();  // Update controller states

    ExtendedController& ctrl = gInputHLE.GetController(0); // Player 1

    // Buttons
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

    // ---------------- Analog Sticks ----------------
    float lx, ly, rx, ry;
    ctrl.GetLeftStick(lx, ly);
    ctrl.GetRightStick(rx, ry);

    // Convert normalized -1.0 -> 1.0 to 16-bit WORD range 0x0000 -> 0xFFFF
    auto StickToWord = [](float f) -> WORD {
        return WORD((std::clamp(f, -1.0f, 1.0f) + 1.0f) * 0.5f * 65535.0f);
    };

    r->GPR[A0*2] = StickToWord(lx); // Example mapping
    r->GPR[A1*2] = StickToWord(ly);
    r->GPR[A2*2] = StickToWord(rx);
    r->GPR[A3*2] = StickToWord(ry);

    // ---------------- Motion Sensors ----------------
    float gx, gy, gz, ax, ay, az;
    ctrl.GetGyro(gx, gy, gz);
    ctrl.GetAccel(ax, ay, az);

    // Pack motion sensor data into emulator registers
    r->FPR[0]  = gx;  // Gyro X
    r->FPR[1]  = gy;  // Gyro Y
    r->FPR[2]  = gz;  // Gyro Z
    r->FPR[3]  = ax;  // Accel X
    r->FPR[4]  = ay;  // Accel Y
    r->FPR[5]  = az;  // Accel Z
}
