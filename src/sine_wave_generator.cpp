#include "sine_wave_generator.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "utils.h"

// Global variables for sine wave generation
bool sineWaveActive = false;
unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 250; // 0.25 seconds in milliseconds

// Sine wave parameters
float sineAmplitude = 5.0;    // Default amplitude
float sinePeriod = 1.0;       // Default period in seconds
float sineOffset = 5.0;       // Center point (user configurable)
unsigned long startTime = 0;   // Start time of sine wave
bool allowOvershoot = false;  // Whether to allow overshoot beyond safe ranges
char sineWaveMode = 'v';      // Current mode: 'v'=voltage, 'c'=current, 'd'=digital

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
    sineWaveActive = false;
    lastUpdateTime = 0;
    Serial.println("Sine Wave Generator initialized (experimental feature)");
}

/**
 * Start sine wave generation
 * @param amplitude: Peak amplitude
 * @param period: Period in seconds (1-60s)
 * @param center: Center point of the sine wave
 * @param signal: Signal number (1-3)
 * @param mode: 'v' for voltage, 'c' for current, 'd' for digital
 * @param overshoot: Whether to allow overshoot beyond safe ranges
 */
void startSineWave(float amplitude, float period, float center, uint8_t signal, char mode, bool overshoot) {
    if (signal < 1 || signal > 3) {
        Serial.println("Invalid signal number. Use 1-3.");
        return;
    }
    
    if (mode != 'v' && mode != 'c' && mode != 'd') {
        Serial.println("Invalid mode. Use 'v' for voltage, 'c' for current, or 'd' for digital.");
        return;
    }
    
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
    
    if (mode == 'd') {
        if (amplitude < 0) {
            Serial.println("Invalid digital amplitude. Use 0 or higher.");
        return;
        }
        
        // For digital mode, center should be around 0.5 (threshold for HIGH/LOW)
        // Amplitude determines the range around center
        float minOutput = center - amplitude;
        float maxOutput = center + amplitude;
        
        Serial.printf("Digital mode: Threshold center=%.2f, amplitude=%.2f\n", center, amplitude);
        Serial.printf("Digital range: %.2f-%.2f (values > 0.5 = HIGH, <= 0.5 = LOW)\n", minOutput, maxOutput);
    }
    
    // Validate period
    if (period < 1.0 || period > 60.0) {
        Serial.println("Invalid period. Use 1-60 seconds.");
        return;
    }
    
    // Set parameters
    sineAmplitude = amplitude;
    sinePeriod = period;
    sineOffset = center; // User-defined center point
    allowOvershoot = overshoot;
    sineWaveMode = mode; // Save the mode
    startTime = millis();
    lastUpdateTime = 0;
    sineWaveActive = true;
    
    // Set signal mode (only for analog modes)
    if (mode != 'd') {
    signalModes[signal - 1] = mode;
    setRelayMode(signal, mode);
    }
    
    const char* modeStr = (mode == 'v') ? "voltage" : (mode == 'c') ? "current" : "digital";
    const char* unitStr = (mode == 'v') ? "V" : (mode == 'c') ? "mA" : "";
    
    Serial.printf("Sine wave started: Signal %d, %s mode\n", signal, modeStr);
    Serial.printf("Amplitude: %.2f%s, Center: %.2f%s, Period: %.1fs\n", 
                  amplitude, unitStr, 
                  center, unitStr, 
                  period);
    
    if (mode == 'd') {
        Serial.printf("Digital threshold: %.2f (values > 0.5 = HIGH, <= 0.5 = LOW)\n", center);
    } else {
        Serial.printf("Output range: %.1f-%.1f%s (will be clamped to safe boundaries)\n", 
                      center - amplitude, center + amplitude, unitStr);
    }
}

/**
 * Stop sine wave generation
 */
void stopSineWave() {
    if (sineWaveActive) {
        sineWaveActive = false;
        Serial.println("Sine wave stopped.");
        
        // Reset all outputs to 0 for safety
        initializeDACs();
        Serial.println("All outputs reset to 0.");
    } else {
        Serial.println("No sine wave is currently active.");
    }
}

/**
 * Update sine wave output (call this in main loop)
 */
void updateSineWave() {
    if (!sineWaveActive) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Check if it's time for next update (every 0.25 seconds)
    if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
        return;
    }
    
    lastUpdateTime = currentTime;
    
    // Calculate elapsed time since start
    unsigned long elapsedTime = currentTime - startTime;
    float timeInSeconds = elapsedTime / 1000.0;
    
    // Calculate sine wave value
    float frequency = 1.0 / sinePeriod;
    float angle = 2.0 * PI * frequency * timeInSeconds;
    float sineValue = sin(angle);
    
    // Calculate output value
    float outputValue = sineOffset + (sineValue * sineAmplitude);
    
    // Handle different output modes
    if (sineWaveMode == 'd') {
        // Digital mode: convert sine wave to HIGH/LOW based on threshold
        bool digitalOutput = (outputValue > 0.5);
        
        // Output digital signal to the active signal
        for (int sig = 1; sig <= 3; sig++) {
            if (signalModes[sig - 1] == 'v' || signalModes[sig - 1] == 'c') {
                // Use voltage channel for digital output (easier to control)
                digitalWrite(sig == 1 ? 15 : (sig == 2 ? 26 : 33), digitalOutput ? HIGH : LOW);
            }
        }
        
        // Update status display for digital mode
        static int updateCounter = 0;
        updateCounter++;
        if (updateCounter >= 4) {
            updateCounter = 0;
            Serial.printf("Digital sine wave: %s at %.1fs (%.1f%% complete)\n", 
                          digitalOutput ? "HIGH" : "LOW",
                          timeInSeconds,
                          (timeInSeconds / sinePeriod) * 100.0);
        }
    } else {
        // Analog mode (voltage/current)
        // Always ensure output is within valid range (limit at boundaries)
    if (outputValue < 0) outputValue = 0;
        
        // Check signal mode to determine limits
        for (int sig = 1; sig <= 3; sig++) {
            if (signalModes[sig - 1] == 'v' || signalModes[sig - 1] == 'c') {
                if (signalModes[sig - 1] == 'v' && outputValue > 10.0) outputValue = 10.0; // Voltage mode
                if (signalModes[sig - 1] == 'c' && outputValue > 25.0) outputValue = 25.0; // Current mode
            }
        }
    
    // Output to all active signals
    for (int sig = 1; sig <= 3; sig++) {
        if (signalModes[sig - 1] == 'v' || signalModes[sig - 1] == 'c') {
            if (signalModes[sig - 1] == 'v') {
                    sineSignalMap[sig - 1].voltageDAC->setVoltage(outputValue, sineSignalMap[sig - 1].voltageChannel);
            } else {
                    sineSignalMap[sig - 1].currentDAC->setDACOutElectricCurrent(static_cast<uint16_t>(outputValue * 1000));
            }
        }
    }
    
        // Print status every 4 updates (every second) for analog mode
    static int updateCounter = 0;
    updateCounter++;
    if (updateCounter >= 4) {
        updateCounter = 0;
            Serial.printf("Sine wave: %.2f%s at %.1fs (%.1f%% complete)\n", 
                          outputValue,
                          (sineWaveMode == 'v') ? "V" : "mA",
                      timeInSeconds,
                      (timeInSeconds / sinePeriod) * 100.0);
    }
    }
    

}

/**
 * Check if sine wave is active
 * @return true if sine wave is active
 */
bool isSineWaveActive() {
    return sineWaveActive;
}

/**
 * Get sine wave status
 */
void getSineWaveStatus() {
    if (sineWaveActive) {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - startTime;
        float timeInSeconds = elapsedTime / 1000.0;
        float progress = (timeInSeconds / sinePeriod) * 100.0;
        
        Serial.println("=== SINE WAVE STATUS ===");
        Serial.printf("Status: ACTIVE\n");
        Serial.printf("Amplitude: %.2f\n", sineAmplitude);
        Serial.printf("Period: %.1f seconds\n", sinePeriod);
        Serial.printf("Elapsed time: %.1f seconds\n", timeInSeconds);
        Serial.printf("Progress: %.1f%%\n", progress);
        Serial.printf("Center point: %.2f\n", sineOffset);
        Serial.printf("Mode: %s\n", (sineWaveMode == 'v') ? "Voltage" : (sineWaveMode == 'c') ? "Current" : "Digital");
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
        stopSineWave();
        
    } else if (input.startsWith("SINE STATUS")) {
        getSineWaveStatus();
        
    } else {
        Serial.println("Invalid sine wave command. Use:");
        Serial.println("  SINE START amplitude period center signal mode");
        Serial.println("  SINE STOP");
        Serial.println("  SINE STATUS");
        Serial.println("Example: SINE START 5.0 2.0 5.0 1 V");
        Serial.println("Example: SINE START 3.0 1.5 2.5 2 C");
        Serial.println("Parameters:");
        Serial.println("  amplitude: Peak amplitude from center");
        Serial.println("  period: Period in seconds (1-60s)");
        Serial.println("  center: Center point of the sine wave");
        Serial.println("  signal: Signal number (1-3)");
        Serial.println("  mode: 'v' for voltage, 'c' for current, 'd' for digital");
        Serial.println("Note: Values exceeding safe ranges will be clamped to boundaries:");
        Serial.println("  Voltage: 0-10V, Current: 0-25mA");
        Serial.println("  Digital: Threshold at center (values > 0.5 = HIGH, <= 0.5 = LOW)");
    }
} 