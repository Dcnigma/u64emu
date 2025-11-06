#include "CEmuObject1.h"
#include "mmDisplay.h"

extern mmDisplay theDisplay;

void CEmuObject::UpdateDisplay() {
    // Just clear screen with a color for now
    unsigned char dummy[320 * 240 * 2] = {0};
    theDisplay.UpdateScreenBuffer(dummy);
}
