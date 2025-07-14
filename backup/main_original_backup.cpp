#ifndef DISABLE_MODBUS
#include "modbus_handler.h"
#endif
#include "command_handler.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "sine_wave_generator.h"

// Function declarations
void selectMode();
void resetAllOutputs();

// Global variables
char signalModes[3] = {'v', 'v', 'v'}; // Default all signals to voltage mode
bool digitalMode = false; // Global flag to track selected mode

// Function to reset all outputs to 0 for safety
void resetAllOutputs() {
    Serial.println("Resetting all outputs to 0 for safety...");
    
    // Reset DAC outputs
    initializeDACs();
    
    // Reset signal modes to default (voltage)
    signalModes[0] = 'v';
    signalModes[1] = 'v';
    signalModes[2] = 'v';
    
    // Reset relay states
    setRelayMode(1, 'v');
    setRelayMode(2, 'v');
    setRelayMode(3, 'v');
    
#ifndef DISABLE_MODBUS
    // Reset Modbus registers if in digital mode
    if (digitalMode) {
        // Clear all Modbus holding registers
        for (int i = 0; i < 4; i++) {
            mb.addHreg(i, 0x01, 1);
            mb.Hreg(i, 0);
        }
        Serial.println("Modbus registers reset to 0.");
    }
#endif
    
    Serial.println("All outputs reset to 0. Safe to switch modes.");
}

void selectMode() {
    Serial.println("=== MODE SELECTION ===");
    Serial.println("Please select your operating mode:");
    Serial.println("D - Digital Mode (Modbus Simulation)");
    Serial.println("A - Analogue Mode (Direct Output Control)");
    Serial.println("Enter 'D' or 'A':");
    
    while (!Serial.available()) {
        delay(100); // Wait for user input
    }
    
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    
    if (input == "D") {
        digitalMode = true;
        Serial.println("Digital Mode (Modbus Simulation) selected.");
#ifndef DISABLE_MODBUS
        // Initialize Modbus
        initModbus();
        Serial.println("Send your command in the format: REGINDEX,REGADDRESS,TYPE,VALUE");
        Serial.println("Types: I - U64, F - Float, S - Int16");
        Serial.println("Example: 0,3059,F,1078.69");
        Serial.println(" - SWITCH - Switch to Analogue Mode");
#else
        Serial.println("Modbus functionality is disabled in this build.");
#endif
    } else if (input == "A") {
        digitalMode = false;
        Serial.println("Analogue Mode (Direct Output Control) selected.");
        Serial.println("Commands (case-insensitive):");
        Serial.println(" - MODE SIG,MODE  (e.g., mode 1,V or MODE 2,C)");
        Serial.println(" - VALUE SIG,VALUE  (e.g., value 1,5.0 or VALUE 2,10.0)");
        Serial.println(" - SINE START/STOP/STATUS - Sine wave generation");
        Serial.println(" - SWITCH - Switch to Digital Mode");
    } else {
        Serial.println("Invalid selection. Please enter 'D' or 'A'.");
        selectMode(); // Recursive call to try again
        return;
    }
    
    Serial.println("System Initialized. Ready for commands.");
}

void setup() {
    // Initialize serial
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial connection
    }

    // Initialize hardware
    Wire.begin(4, 0);      // Initialize I2C: SDA = IO4, SCL = IO0
    initRelayController(); // Initialize relay controller
    initializeDACs();      // Initialize DAC outputs, set to 0
    initSineWaveGenerator(); // Initialize sine wave generator

    // Mode selection
    selectMode();
}

void loop() {
    // Check if there's user input from serial
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n'); // Read one line of user input
        input.trim(); // Remove leading and trailing spaces

        // Convert to uppercase for case-insensitive comparison
        String upperInput = input;
        upperInput.toUpperCase();

        if (digitalMode) {
            // Digital mode - Modbus commands only
            if (upperInput.startsWith("SWITCH")) {
                Serial.println("Switching to Analogue Mode...");
                // Reset all outputs to 0 for safety
                resetAllOutputs();
                digitalMode = false;
                Serial.println("Analogue Mode (Direct Output Control) activated.");
                Serial.println("Commands (case-insensitive):");
                Serial.println(" - MODE SIG,MODE  (e.g., mode 1,V or MODE 2,C)");
                Serial.println(" - VALUE SIG,VALUE  (e.g., value 1,5.0 or VALUE 2,10.0)");
                Serial.println(" - SINE START/STOP/STATUS - Sine wave generation");
                Serial.println(" - SWITCH - Switch back to Digital Mode");
            } else {
#ifndef DISABLE_MODBUS
                processInput(input);
#else
                Serial.println("Modbus functionality is disabled in this build.");
#endif
            }
        } else {
            // Analogue mode - Direct control commands only
            if (upperInput.startsWith("MODE")) {
                parseModeCommand(input.substring(5)); // Parse MODE command
            } else if (upperInput.startsWith("VALUE")) {
                parseValueCommand(input.substring(6)); // Parse VALUE command
            } else if (upperInput.startsWith("SINE")) {
                parseSineWaveCommand(input); // Parse SINE command
            } else if (upperInput.startsWith("SWITCH")) {
                Serial.println("Switching to Digital Mode...");
                // Reset all outputs to 0 for safety
                resetAllOutputs();
                digitalMode = true;
#ifndef DISABLE_MODBUS
                Serial.println("Digital Mode (Modbus Simulation) activated.");
                Serial.println("Send your command in the format: REGINDEX,REGADDRESS,TYPE,VALUE");
                Serial.println("Types: I - U64, F - Float, S - Int16");
                Serial.println("Example: 0,3059,F,1078.69");
                Serial.println(" - SWITCH - Switch back to Analogue Mode");
#else
                Serial.println("Modbus functionality is disabled in this build.");
#endif
            } else {
                Serial.println("Invalid command for Analogue mode. Use 'MODE', 'VALUE', 'SINE', or 'SWITCH' (case-insensitive).");
            }
        }
    }

    if (digitalMode) {
#ifndef DISABLE_MODBUS
        // Handle Modbus tasks
        mb.task();
#endif
    } else {
        // Update sine wave in analogue mode
        updateSineWave();
    }

    delay(10); // Avoid loop running too fast
} 