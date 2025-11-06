#include "EmuObject1.h"

CEmuObject::CEmuObject() {
    m_FileName[0] = '\0';
    m_InputDevice = nullptr;
    m_Debug = false;
    m_IsRunning = false;
    m_CycleCount = 0;
    m_FrameCount = 0;
}

CEmuObject::~CEmuObject() {}

// Core functions
void CEmuObject::Init() {}
void CEmuObject::Emulate(const char* filename) { (void)filename; }
bool CEmuObject::UpdateDisplay() { return true; }
void CEmuObject::StopEmulation() {}
void CEmuObject::UpdateAudio(unsigned int size) { (void)size; }

// Optional game functions
void CEmuObject::LoadGame(const char* filename) { (void)filename; }
void CEmuObject::SaveGame(const char* filename) { (void)filename; }
void CEmuObject::Reset() {}
