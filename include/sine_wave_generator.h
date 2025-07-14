#ifndef SINE_WAVE_GENERATOR_H
#define SINE_WAVE_GENERATOR_H

#include <Arduino.h>

// Experimental Sine Wave Generator
// This feature allows generation of sinusoidal waves with configurable parameters
// Resolution: 0.25 seconds
// Period range: 1-60 seconds
// Amplitude and center point: User configurable
// Output modes: Voltage (0-10V), Current (0-25mA), Digital (HIGH/LOW)
// Safe ranges: Voltage 0-10V, Current 0-25mA (values are clamped to boundaries)

/**
 * Initialize sine wave generator
 */
void initSineWaveGenerator();

/**
 * Start sine wave generation
 * @param amplitude: Peak amplitude from center point
 * @param period: Period in seconds (1-60s)
 * @param center: Center point of the sine wave
 * @param signal: Signal number (1-3)
 * @param mode: 'v' for voltage, 'c' for current, 'd' for digital
 * @param overshoot: Unused parameter (kept for compatibility)
 */
void startSineWave(float amplitude, float period, float center, uint8_t signal, char mode, bool overshoot = false);

/**
 * Stop sine wave generation
 */
void stopSineWave();

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
 * Parse sine wave commands
 * Format: SINE START/STOP/STATUS [amplitude] [period] [signal] [mode]
 */
void parseSineWaveCommand(String input);

// Command examples:
// SINE START 5.0 2.0 5.0 1 V    // Start 5V amplitude, 2s period, center 5V, signal 1, voltage mode (output: 0-10V)
// SINE START 3.0 1.5 2.5 2 C   // Start 3mA amplitude, 1.5s period, center 2.5mA, signal 2, current mode (output: 0-5.5mA, clamped)
// SINE START 0.5 1.0 0.5 3 D   // Start digital sine wave, 1s period, threshold 0.5, signal 3, digital mode (HIGH/LOW)
// SINE STOP                  // Stop sine wave generation
// SINE STATUS               // Get current status

extern char signalModes[3];

#endif // SINE_WAVE_GENERATOR_H 