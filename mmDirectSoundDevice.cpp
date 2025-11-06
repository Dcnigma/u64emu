#include "mmDirectSoundDevice.h"

// No audio on Switch yet, stub everything
mmDirectSoundDevice::mmDirectSoundDevice() {}
mmDirectSoundDevice::~mmDirectSoundDevice() {}
HRESULT mmDirectSoundDevice::Create(WORD MachineNum) { return 0; }
BOOL mmDirectSoundDevice::Play() { return true; }
void mmDirectSoundDevice::ErrorMessage(HRESULT error, char *string)
{
    strcpy(string, "DirectSound not supported on Switch");
}
