#include "sine_wave_generator.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "utils.h"

// Global variables for sine wave generation
bool sineWaveActive[3] = {false, false, false}; // Per-channel sine wave status
unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 250; // 0.25 seconds in milliseconds

// Sine wave parameters per channel
float sineAmplitude[3] = {5.0, 5.0, 5.0};    // Default amplitude per channel
float sinePeriod[3] = {1.0, 1.0, 1.0};       // Default period in seconds per channel
float sineOffset[3] = {5.0, 5.0, 5.0};       // Center point per channel
unsigned long startTime[3] = {0, 0, 0};       // Start time per channel
char sineWaveMode[3] = {'v', 'v', 'v'};       // Current mode per channel: 'v'=voltage, 'c'=current

// Signal mapping for sine wave output
extern char signalModes[3];

// Local signal mapping table for sine wave generator
SineSignalMap sineSignalMap[3] = {
    {&gp8413_1, 0, &gp8313_1}, // SIG1
    {&gp8413_1, 1, &gp8313_2}, // SIG2
    {&gp8413_2, 0, &gp8313_3}  // SIG3
};

/**
 * Initialize sine wave generator
 */
void initSineWaveGenerator() {
    for (int i = 0; i < 3; i++) {
        sineWaveActive[i] = false;
        sineAmplitude[i] = 5.0;
        sinePeriod[i] = 1.0;
        sineOffset[i] = 5.0;
        startTime[i] = 0;
        sineWaveMode[i] = 'v';
    }
    lastUpdateTime = 0;
    Serial.println("Sine Wave Generator initialized (analog mode only)");
}

/**
 * Start sine wave generation
 * @param amplitude: Peak amplitude
 * @param period: Period in seconds (1-60s)
 * @param center: Center point of the sine wave
 * @param signal: Signal number (1-3)
 * @param mode: 'v' for voltage, 'c' for current
 * @param overshoot: Whether to allow overshoot beyond safe ranges
 */
void startSineWave(float amplitude, float period, float center, uint8_t signal, char mode, bool overshoot) {
    if (signal < 1 || signal > 3) {
        Serial.println("Invalid signal number. Use 1-3.");
        return;
    }
    
    if (mode != 'v' && mode != 'c') {
        Serial.println("Invalid mode. Use 'v' for voltage or 'c' for current.");
        return;
    }
    
    int channel = signal - 1; // Convert to 0-based index
    
    // Validate amplitude and center point based on mode
    if (mode == 'v') {
        if (amplitude < 0) {
            Serial.println("Invalid voltage amplitude. Use 0 or higher.");
            return;
        }
        
        // Calculate output range
        float minOutput = center - amplitude;
        float maxOutput = center + amplitude;
        
        // Warn if output range exceeds safe limits (but don't block - will be clamped at runtime)
        if (minOutput < 0 || maxOutput > 10.0) {
            Serial.printf("Warning: Output range %.1f-%.1fV exceeds 0-10V safe range.\n", minOutput, maxOutput);
            Serial.println("Values will be clamped to safe boundaries during generation.");
        }
    }
    
    if (mode == 'c') {
        if (amplitude < 0) {
            Serial.println("Invalid current amplitude. Use 0 or higher.");
            return;
        }
        
        // Calculate output range
        float minOutput = center - amplitude;
        float maxOutput = center + amplitude;
        
        // Warn if output range exceeds safe limits (but don't block - will be clamped at runtime)
        if (minOutput < 0 || maxOutput > 25.0) {
            Serial.printf("Warning: Output range %.1f-%.1fmA exceeds 0-25mA safe range.\n", minOutput, maxOutput);
            Serial.println("Values will be clamped to safe boundaries during generation.");
        }
    }
    
    // Validate period
    if (period < 1.0 || period > 60.0) {
        Serial.println("Invalid period. Use 1-60 seconds.");
        return;
    }
    
    // Set parameters for this channel
    sineAmplitude[channel] = amplitude;
    sinePeriod[channel] = period;
    sineOffset[channel] = center;
    sineWaveMode[channel] = mode;
    startTime[channel] = millis();
    sineWaveActive[channel] = true;
    lastUpdateTime = 0;
    
    // Set signal mode and relay
    signalModes[signal - 1] = mode;
    setRelayMode(signal, mode);
    
    Serial.printf("Sine wave started on SIG%d: %.2f%s amplitude, %.1fs period, center %.2f%s, %s mode\n",
                  signal,
                  amplitude,
                  (mode == 'v') ? "V" : "mA",
                  period,
                  center,
                  (mode == 'v') ? "V" : "mA",
                  (mode == 'v') ? "voltage" : "current");
}

/**
 * Stop sine wave generation for a specific channel
 * @param signal: Signal number (1-3), 0 to stop all channels
 */
void stopSineWave(uint8_t signal) {
    if (signal == 0) {
        // Stop all channels
        bool anyActive = false;
        for (int i = 0; i < 3; i++) {
            if (sineWaveActive[i]) {
                sineWaveActive[i] = false;
                anyActive = true;
            }
        }
        
        if (anyActive) {
            Serial.println("All sine waves stopped.");
            // Reset all outputs to 0 for safety
            initializeDACs();
            Serial.println("All outputs reset to 0.");
        } else {
            Serial.println("No sine waves are currently active.");
        }
    } else if (signal >= 1 && signal <= 3) {
        // Stop specific channel
        int channel = signal - 1;
        if (sineWaveActive[channel]) {
            sineWaveActive[channel] = false;
            Serial.printf("Sine wave stopped on SIG%d.\n", signal);
            
            // Reset this channel's output to 0
            if (sineWaveMode[channel] == 'v') {
                sineSignalMap[channel].voltageDAC->setVoltage(0.0, sineSignalMap[channel].voltageChannel);
            } else if (sineWaveMode[channel] == 'c') {
                sineSignalMap[channel].currentDAC->setDACOutElectricCurrent(0);
            }
            Serial.printf("SIG%d output reset to 0.\n", signal);
        } else {
            Serial.printf("No sine wave is active on SIG%d.\n", signal);
        }
    } else {
        Serial.println("Invalid signal number. Use 1-3, or 0 to stop all.");
    }
}

/**
 * Update sine wave output (call this in main loop)
 */
void updateSineWave() {
    unsigned long currentTime = millis();
    
    // Check if it's time for next update (every 0.25 seconds)
    if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
        return;
    }
    
    lastUpdateTime = currentTime;
    
    // Process each active channel
    for (int channel = 0; channel < 3; channel++) {
        if (!sineWaveActive[channel]) {
            continue;
        }
        
        // Calculate elapsed time since start for this channel
        unsigned long elapsedTime = currentTime - startTime[channel];
        float timeInSeconds = elapsedTime / 1000.0;
        
        // Calculate sine wave value for this channel
        float frequency = 1.0 / sinePeriod[channel];
        float angle = 2.0 * PI * frequency * timeInSeconds;
        float sineValue = sin(angle);
        
        // Calculate output value for this channel
        float outputValue = sineOffset[channel] + (sineValue * sineAmplitude[channel]);
        
        // Clamp output to safe ranges
        if (outputValue < 0) outputValue = 0;
        if (sineWaveMode[channel] == 'v' && outputValue > 10.0) outputValue = 10.0;
        if (sineWaveMode[channel] == 'c' && outputValue > 25.0) outputValue = 25.0;
        
        // Output to this channel
        if (sineWaveMode[channel] == 'v') {
            sineSignalMap[channel].voltageDAC->setVoltage(outputValue, sineSignalMap[channel].voltageChannel);
        } else if (sineWaveMode[channel] == 'c') {
            // Convert mA to DAC data: Rset=2kΩ, 25mA = 32767 (15-bit), so 1mA = 1310.68
            sineSignalMap[channel].currentDAC->setDACOutElectricCurrent(static_cast<uint16_t>(outputValue * 1310.68));
        }
        
        // Status printing removed - use 'SINE STATUS' command to check progress
    }
}

/**
 * Check if sine wave is active on any channel
 * @return true if any sine wave is active
 */
bool isSineWaveActive() {
    for (int i = 0; i < 3; i++) {
        if (sineWaveActive[i]) {
            return true;
        }
    }
    return false;
}

/**
 * Check if sine wave is active on specific channel
 * @param channel: Channel number (0-2)
 * @return true if sine wave is active on this channel
 */
bool isSineWaveActiveOnChannel(uint8_t channel) {
    if (channel >= 3) return false;
    return sineWaveActive[channel];
}

/**
 * Get sine wave parameters for specific channel
 * @param channel: Channel number (0-2)
 * @param amplitude: Output parameter for amplitude
 * @param period: Output parameter for period
 * @param center: Output parameter for center point
 * @param mode: Output parameter for mode ('v' or 'c')
 * @return true if sine wave is active on this channel
 */
bool getSineWaveParams(uint8_t channel, float* amplitude, float* period, float* center, char* mode) {
    if (channel >= 3 || !sineWaveActive[channel]) {
        return false;
    }
    
    *amplitude = sineAmplitude[channel];
    *period = sinePeriod[channel];
    *center = sineOffset[channel];
    *mode = sineWaveMode[channel];
    
    return true;
}

/**
 * Get sine wave status
 */
void getSineWaveStatus() {
    bool anyActive = false;
    
    for (int i = 0; i < 3; i++) {
        if (sineWaveActive[i]) {
            if (!anyActive) {
                Serial.println("=== SINE WAVE STATUS ===");
                anyActive = true;
            }
            
            unsigned long currentTime = millis();
            unsigned long elapsedTime = currentTime - startTime[i];
            float timeInSeconds = elapsedTime / 1000.0;
            float progress = (timeInSeconds / sinePeriod[i]) * 100.0;
            
            Serial.printf("SIG%d: ACTIVE\n", i + 1);
            Serial.printf("  Amplitude: %.2f%s\n", sineAmplitude[i], (sineWaveMode[i] == 'v') ? "V" : "mA");
            Serial.printf("  Period: %.1f seconds\n", sinePeriod[i]);
            Serial.printf("  Elapsed time: %.1f seconds\n", timeInSeconds);
            Serial.printf("  Progress: %.1f%%\n", progress);
            Serial.printf("  Center point: %.2f%s\n", sineOffset[i], (sineWaveMode[i] == 'v') ? "V" : "mA");
            Serial.printf("  Mode: %s\n", (sineWaveMode[i] == 'v') ? "Voltage" : "Current");
            Serial.println();
        }
    }
    
    if (anyActive) {
        Serial.println("========================");
    } else {
        Serial.println("Sine wave: INACTIVE");
    }
}

/**
 * Parse sine wave commands
 * Format: SINE START/STOP/STATUS [amplitude] [period] [signal] [mode]
 */
void parseSineWaveCommand(String input) {
    input.trim();
    input.toUpperCase();
    
    if (input.startsWith("SINE START")) {
        // Parse: SINE START amplitude period center signal mode
        // Example: SINE START 5.0 2.0 5.0 1 V
        // Example: SINE START 3.0 1.5 2.5 2 C
        String params = input.substring(11); // Remove "SINE START "
        params.trim();
        
        // Parse parameters
        int space1 = params.indexOf(' ');
        int space2 = params.indexOf(' ', space1 + 1);
        int space3 = params.indexOf(' ', space2 + 1);
        int space4 = params.indexOf(' ', space3 + 1);
        
        if (space1 == -1 || space2 == -1 || space3 == -1 || space4 == -1) {
            Serial.println("Invalid SINE START format. Use: SINE START amplitude period center signal mode");
            Serial.println("Example: SINE START 5.0 2.0 5.0 1 V");
            Serial.println("Example: SINE START 3.0 1.5 2.5 2 C");
            return;
        }
        
        float amplitude = params.substring(0, space1).toFloat();
        float period = params.substring(space1 + 1, space2).toFloat();
        float center = params.substring(space2 + 1, space3).toFloat();
        uint8_t signal = params.substring(space3 + 1, space4).toInt();
        char mode = toLowerCase(params.substring(space4 + 1).charAt(0));
        
        startSineWave(amplitude, period, center, signal, mode, false);
        
    } else if (input.startsWith("SINE STOP")) {
        String params = input.substring(10); // Remove "SINE STOP "
        params.trim();
        
        if (params.length() == 0) {
            // Stop all channels
            stopSineWave(0);
        } else {
            // Stop specific channel
            uint8_t signal = params.toInt();
            if (signal >= 1 && signal <= 3) {
                stopSineWave(signal);
            } else {
                Serial.println("Invalid signal number. Use 1-3, or no parameter to stop all.");
            }
        }
        
    } else if (input.startsWith("SINE STATUS")) {
        getSineWaveStatus();
        
    } else {
        Serial.println("Invalid sine wave command. Use:");
        Serial.println("  SINE START amplitude period center signal mode");
        Serial.println("  SINE STOP [signal]");
        Serial.println("  SINE STATUS");
        Serial.println("Examples:");
        Serial.println("  SINE START 5.0 2.0 5.0 1 V    // Start voltage sine wave on SIG1");
        Serial.println("  SINE START 3.0 1.5 2.5 2 C    // Start current sine wave on SIG2");
        Serial.println("  SINE STOP                     // Stop all sine waves");
        Serial.println("  SINE STOP 1                   // Stop sine wave on SIG1 only");
        Serial.println("Parameters:");
        Serial.println("  amplitude: Peak amplitude from center");
        Serial.println("  period: Period in seconds (1-60s)");
        Serial.println("  center: Center point of the sine wave");
        Serial.println("  signal: Signal number (1-3)");
        Serial.println("  mode: 'v' for voltage, 'c' for current");
        Serial.println("Note: Values exceeding safe ranges will be clamped to boundaries:");
        Serial.println("  Voltage: 0-10V, Current: 0-25mA");
    }
} 