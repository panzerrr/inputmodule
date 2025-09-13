#ifndef SINE_WAVE_GENERATOR_H
#define SINE_WAVE_GENERATOR_H

#include <Arduino.h>

// Sine Wave Generator (Analog Mode Only)
// This feature allows generation of sinusoidal waves with configurable parameters
// Resolution: 0.25 seconds
// Period range: 1-60 seconds
// Amplitude and center point: User configurable
// Output modes: Voltage (0-10V), Current (0-25mA)
// Safe ranges: Voltage 0-10V, Current 0-25mA (values are clamped to boundaries)
// Multi-channel support: Each signal can have independent sine wave parameters

/**
 * Initialize sine wave generator
 */
void initSineWaveGenerator();

/**
 * Start sine wave generation for a specific channel
 * @param amplitude: Peak amplitude from center point
 * @param period: Period in seconds (1-60s)
 * @param center: Center point of the sine wave
 * @param signal: Signal number (1-3)
 * @param mode: 'v' for voltage, 'c' for current
 * @param overshoot: Unused parameter (kept for compatibility)
 */
void startSineWave(float amplitude, float period, float center, uint8_t signal, char mode, bool overshoot);

/**
 * Stop sine wave generation
 * @param signal: Signal number (1-3), 0 to stop all channels
 */
void stopSineWave(uint8_t signal);

/**
 * Update sine wave output (call this in main loop)
 */
void updateSineWave();

/**
 * Get sine wave status
 */
void getSineWaveStatus();

/**
 * Check if sine wave is active
 * @return true if sine wave is active
 */
bool isSineWaveActive();

/**
 * Check if sine wave is active on specific channel
 * @param channel: Channel number (0-2)
 * @return true if sine wave is active on this channel
 */
bool isSineWaveActiveOnChannel(uint8_t channel);

/**
 * Get sine wave parameters for specific channel
 * @param channel: Channel number (0-2)
 * @param amplitude: Output parameter for amplitude
 * @param period: Output parameter for period
 * @param center: Output parameter for center point
 * @param mode: Output parameter for mode ('v' or 'c')
 * @return true if sine wave is active on this channel
 */
bool getSineWaveParams(uint8_t channel, float* amplitude, float* period, float* center, char* mode);

/**
 * Parse sine wave commands
 * Format: SINE START/STOP/STATUS [amplitude] [period] [center] [signal] [mode]
 */
void parseSineWaveCommand(String input);

// Command examples:
// SINE START 5.0 2.0 5.0 1 V    // Start 5V amplitude, 2s period, center 5V, signal 1, voltage mode (output: 0-10V)
// SINE START 3.0 1.5 2.5 2 C    // Start 3mA amplitude, 1.5s period, center 2.5mA, signal 2, current mode (output: 0-5.5mA, clamped)
// SINE STOP                     // Stop all sine wave generation
// SINE STOP 1                   // Stop sine wave on signal 1 only
// SINE STATUS                   // Get current status

extern char signalModes[3];

#endif // SINE_WAVE_GENERATOR_H 