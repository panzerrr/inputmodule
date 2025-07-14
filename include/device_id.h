#ifndef DEVICE_ID_H
#define DEVICE_ID_H

#include <Arduino.h>

// Define ID pins - Hardware jumpers/switches for device identification
#define NO1 23  // IO23 - Device ID Bit 0
#define NO2 12  // IO12 - Device ID Bit 1  
#define NO3 4   // IO4  - Device ID Bit 2
#define NO4 5   // IO5  - Device ID Bit 3
#define NO5 32  // IO32 - Device ID Bit 4 (optional)
/**
 * @brief Initialize ID pins
 */
void initDeviceIDPins();

/**
 * @brief Calculate device ID
 * @return Returns the calculated device ID
 */
uint8_t calculateDeviceID();

#endif // DEVICE_ID_H
