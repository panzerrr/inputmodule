#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "dac_controller.h"

// Forward declarations for external DAC objects
extern GP8413 gp8413_1;
extern GP8413 gp8413_2;
extern GP8313 gp8313_1;
extern GP8313 gp8313_2;
extern GP8313 gp8313_3;

// Signal mapping structure for command handler
struct SignalMap {
    GP8413* voltageDAC;
    uint8_t voltageChannel;
    GP8313* currentDAC;
};

// Signal mapping structure for sine wave generator
struct SineSignalMap {
    GP8413* voltageDAC;
    uint8_t voltageChannel;
    GP8313* currentDAC;
};

// Global signal mapping table for command handler
extern SignalMap signalMap[3];

// Global signal mapping table for sine wave generator
extern SineSignalMap sineSignalMap[3];

// Helper function to convert character to lowercase
inline char toLowerCase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32; // Convert to lowercase
    }
    return c;
}

#endif // UTILS_H 