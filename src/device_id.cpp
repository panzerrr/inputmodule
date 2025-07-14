#include "device_id.h"

/**
 * Initialize ID pins
 * Sets up hardware jumper/switch pins for device identification
 * Pull-up resistors ensure HIGH when jumper is open, LOW when jumper is closed
 */
void initDeviceIDPins() {
    pinMode(NO1, INPUT_PULLUP); // IO23 - Device ID Bit 0
    pinMode(NO2, INPUT_PULLUP); // IO12 - Device ID Bit 1
    pinMode(NO3, INPUT_PULLUP); // IO4  - Device ID Bit 2
    pinMode(NO4, INPUT_PULLUP); // IO5  - Device ID Bit 3
    pinMode(NO5, INPUT_PULLUP); // IO32 - Device ID Bit 4 (optional)
}

/**
 * Calculate device ID from hardware jumpers/switches
 * @return Returns the calculated device ID (0-31 for 5-bit, 0-15 for 4-bit)
 * 
 * Hardware setup:
 * - GPIO → 电阻 → GND (接地) = LOW = Bit set to 1 (有效位)
 * - GPIO → 悬空 (上拉) = HIGH = Bit set to 0 (无效位)
 * 
 * ID calculation: NO5 NO4 NO3 NO2 NO1 (5-bit binary)
 */
uint8_t calculateDeviceID() {
    uint8_t id = 0;
    if (digitalRead(NO1) == LOW) id |= 0x01;  // Bit 0 - IO23 (接地=有效=1)
    if (digitalRead(NO2) == LOW) id |= 0x02;  // Bit 1 - IO12 (接地=有效=1)
    if (digitalRead(NO3) == LOW) id |= 0x04;  // Bit 2 - IO4  (接地=有效=1)
    if (digitalRead(NO4) == LOW) id |= 0x08;  // Bit 3 - IO5  (接地=有效=1)
    if (digitalRead(NO5) == LOW) id |= 0x10;  // Bit 4 - IO32 (接地=有效=1)
    return id;
}
