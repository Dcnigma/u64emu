#ifndef iATA_H
#define iATA_H

#include <cstdint>  // for uint8_t

// Type definitions
typedef uint8_t BYTE;

// Function declarations
void iATAConstruct();
void iATADestruct();
bool iATAOpen();
void iATAClose();
void iATAUpdate();
void iATAInitParams();
void iATAReadSectors();
void iATAWriteSectors();
void iATADriveIdentify();
BYTE *iATADataRead();

#endif // iATA_H
