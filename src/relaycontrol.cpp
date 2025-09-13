#include "relay_controller.h"

// Solid state relay pin definitions
#define SW11 14  // SIG1 current (changed from GPIO2 to GPIO14)
#define SW12 15  // SIG1 voltage
#define SW21 27  // SIG2 current
#define SW22 26  // SIG2 voltage
#define SW31 25  // SIG3 current
#define SW32 33  // SIG3 voltage

// Global variables to track relay states
static bool relayStates[6] = {false, false, false, false, false, false}; // Index 0 unused, 1-6 for relays

/**
 * Initialize solid state relays
 */
void initRelayController() {
    // Set all relay pins to output mode
    pinMode(SW11, OUTPUT);
    pinMode(SW12, OUTPUT);
    pinMode(SW21, OUTPUT);
    pinMode(SW22, OUTPUT);
    pinMode(SW31, OUTPUT);
    pinMode(SW32, OUTPUT);

    // Initial state set to all relays off (HIGH = off for these relays)
    digitalWrite(SW11, HIGH);
    digitalWrite(SW12, HIGH);
    digitalWrite(SW21, HIGH);
    digitalWrite(SW22, HIGH);
    digitalWrite(SW31, HIGH);
    digitalWrite(SW32, HIGH);

    Serial.println("Relay Controller Initialized");
}

/**
 * Set SIG mode
 * @param sig: Signal number (1, 2, 3 corresponds to SIG1, SIG2, SIG3)
 * @param mode: Mode ('v' for voltage, 'c' for current)
 */
void setRelayMode(uint8_t sig, char mode) {
    switch (sig) {
        case 1: // SIG1
            digitalWrite(SW11, mode == 'c' ? LOW: HIGH); // Current mode
            digitalWrite(SW12, mode == 'v' ? LOW: HIGH); // Voltage mode
            // Update relay states array
            relayStates[1] = (mode == 'c'); // SW11 (relay 1)
            relayStates[2] = (mode == 'v'); // SW12 (relay 2)
            break;

        case 2: // SIG2
            digitalWrite(SW21, mode == 'c' ? LOW: HIGH);
            digitalWrite(SW22, mode == 'v' ? LOW: HIGH);
            // Update relay states array
            relayStates[3] = (mode == 'c'); // SW21 (relay 3)
            relayStates[4] = (mode == 'v'); // SW22 (relay 4)
            break;

        case 3: // SIG3
            digitalWrite(SW31, mode == 'c' ? LOW: HIGH);
            digitalWrite(SW32, mode == 'v' ? LOW: HIGH);
            // Update relay states array
            relayStates[5] = (mode == 'c'); // SW31 (relay 5)
            relayStates[6] = (mode == 'v'); // SW32 (relay 6)
            break;

        default:
            Serial.println("Invalid signal number. Use 1 to 3.");
            return;
    }

    Serial.printf("Relay mode set: SIG%d -> %c\n", sig, mode);
}

/**
 * Set relay state
 * @param relayNumber Relay number (1-6)
 * @param state true for ON, false for OFF
 */
void setRelay(uint8_t relayNumber, bool state) {
    if (relayNumber < 1 || relayNumber > 6) {
        Serial.printf("Invalid relay number: %d (use 1-6)\n", relayNumber);
        return;
    }
    
    relayStates[relayNumber] = state;
    
    // Map relay numbers to actual pins
    int pin;
    switch (relayNumber) {
        case 1: pin = SW11; break; // SIG1 current
        case 2: pin = SW12; break; // SIG1 voltage
        case 3: pin = SW21; break; // SIG2 current
        case 4: pin = SW22; break; // SIG2 voltage
        case 5: pin = SW31; break; // SIG3 current
        case 6: pin = SW32; break; // SIG3 voltage
        default: return;
    }
    
    digitalWrite(pin, state ? HIGH : LOW);
    Serial.printf("Relay %d set to %s\n", relayNumber, state ? "ON" : "OFF");
}

/**
 * Get relay state
 * @param relayNumber Relay number (1-6)
 * @return true if relay is ON, false if OFF
 */
bool getRelayState(uint8_t relayNumber) {
    if (relayNumber < 1 || relayNumber > 6) {
        return false;
    }
    return relayStates[relayNumber];
}
