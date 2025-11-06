#ifndef MMDISPLAY_H
#define MMDISPLAY_H

#include <cstdint>

class mmDisplay
{
public:
    mmDisplay();
    ~mmDisplay();

    uint16_t m_Width;
    uint16_t m_Height;
    uint16_t m_Depth;

    uint32_t m_FrameCount;
    uint16_t m_ViewWidth;
    uint16_t m_ViewHeight;

    bool m_IsOpen;

    // Functions
    bool RenderScene();
    void ClearScreen();
    void BeginScene();
    void EndScene();
    void Close();
    void UpdateScreenBuffer(unsigned char* source);
    void MakeScreenBuffer();
    int Open(uint16_t Width, uint16_t Height);
};

#endif // MMDISPLAY_H
