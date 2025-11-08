#ifndef SC_SoundBuffer_H
#define SC_SoundBuffer_H

#include <vector>
#include <cstdint>

extern "C" {
#include <switch.h>
}

class mmSoundBuffer
{
public:
    mmSoundBuffer();
    ~mmSoundBuffer();

    // Basic audio buffer info
    uint16_t m_ThisMachine;
    uint16_t m_TotalLength;
    uint16_t m_BufLength;
    bool m_Is3D;

    // Simple file handling using libnx
    FsFile m_File;                  // libnx file handle
    std::vector<uint8_t> m_Data;    // Raw audio data

    // Methods
    bool Create();
    bool FromFile(const char* filename, bool is3D = false);
    bool FromMemory(const uint8_t* src, uint32_t length, uint32_t rate, uint16_t bits, uint16_t buffers, bool is3D = false);
    bool Update(const uint8_t* src, uint32_t length, uint16_t pos);
    bool Play(uint32_t from, uint32_t to);
    bool SetVolume(float vol);
    bool Stop();
    bool IsPlaying() const;

private:
    uint32_t m_Frequency;
    float m_Volume;
    bool m_Playing;
};

#endif
