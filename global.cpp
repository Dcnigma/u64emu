#include "global.h"

// Define the SRAM buffer
unsigned char SRAM[SRAM_SIZE];

// Example getTime
#include <time.h>
float getTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.f + ts.tv_nsec / 1000000.f; // milliseconds
}
