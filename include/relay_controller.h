#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>

/**
 * Initialize solid state relay pins
 * 
 * Configure all solid state relay control pins as output mode and set their initial state to off (LOW).
 */
void initRelayController();

/**
 * Set relay channel working mode
 * @param channel Channel number (1, 2, 3)
 * @param mode Working mode
 *             - 'v': Voltage mode (connect to 8413 output)
 *             - 'c': Current mode (connect to 8313 output)
 *
 * @note
 * - Ensure only one relay is HIGH at the same time to avoid conflicts.
 * - Invalid channel numbers (not 1, 2, 3) will be directly ignored and an error message will be output to the serial port.
 */
void setRelayMode(uint8_t channel, char mode);

/**
 * Set relay state
 * @param relayNumber Relay number (1-6)
 * @param state true for ON, false for OFF
 */
void setRelay(uint8_t relayNumber, bool state);

/**
 * Get relay state
 * @param relayNumber Relay number (1-6)
 * @return true if relay is ON, false if OFF
 */
bool getRelayState(uint8_t relayNumber);

#endif // RELAY_CONTROLLER_H
