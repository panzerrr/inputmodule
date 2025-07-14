#include <Arduino.h>
#include "rs485_serial.h"
#include "rs485_command_handler.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "sine_wave_generator.h"
#include "device_id.h"

char signalModes[3] = {'v', 'v', 'v'};
// Timing variables
unsigned long lastStatusReport = 0;
const unsigned long STATUS_REPORT_INTERVAL = 5000; // 5 seconds

// Forward declarations
void printStatusReport();
void printHelp();
void handleUSBSerialCommands();
void sendTestRS485Command(uint8_t commandType, const uint8_t* data, uint8_t length);

void setup() {
    // Initialize USB Serial for debugging
    Serial.begin(115200);
    Serial.println("=== ESP32 Input Module with RS-485 ===");
    
    // Initialize device ID
    initDeviceIDPins();
    uint8_t deviceID = calculateDeviceID();
    Serial.printf("Device ID: %d\n", deviceID);
    
    // Initialize DAC controllers
    initDACControllers();
    Serial.println("DAC controllers initialized");
    
    // Initialize relay controller
    initRelayController();
    Serial.println("Relay controller initialized");

    // 开机默认三路电压模式
    setRelayMode(1, 'v');
    setRelayMode(2, 'v');
    setRelayMode(3, 'v');
    
    // Initialize sine wave generator
    initSineWaveGenerator();
    Serial.println("Sine wave generator initialized");
    
    // Initialize RS-485 serial communication
    initRS485Serial();
    
    // Initialize RS-485 command handler
    initRS485CommandHandler();
    
    Serial.println("System initialization complete");
    Serial.println("USB Serial: Debug output only");
    Serial.println("RS-485 Serial: Command interface (GPIO 19=TX, 18=RX, 21=DE)");
    Serial.println("Ready to receive commands...");
}

void loop() {
    // Process USB Serial commands
    handleUSBSerialCommands();
    
    // Process RS-485 commands
    if (handleRS485Commands()) {
        // Command was processed, no need to do anything else
        // The command handler will send responses automatically
    }
    
    // Update sine wave generator
    updateSineWave();
    
    // Periodic status report via USB Serial (for debugging)
    if (millis() - lastStatusReport >= STATUS_REPORT_INTERVAL) {
        printStatusReport();
        lastStatusReport = millis();
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

/**
 * Print periodic status report via USB Serial
 */
void printStatusReport() {
    Serial.println("\n=== Status Report ===");
    
    // Device information
    Serial.printf("Device ID: %d\n", getCurrentDeviceID());
    
    // DAC outputs
    Serial.printf("Voltage Output: %.2fV\n", getCurrentVoltage());
    Serial.printf("Current Output: %.2fmA\n", getCurrentCurrent());
    

    
    // Sine wave status
    if (isSineWaveActive()) {
        Serial.println("Sine Wave: ACTIVE");
    } else {
        Serial.println("Sine Wave: INACTIVE");
    }
    
    // RS-485 status
    Serial.printf("RS-485: %s\n", isRS485Available() ? "Data Available" : "Idle");
    
    Serial.println("==================\n");
}

/**
 * Example of how to send a command via RS-485 from USB Serial
 * This function can be called from USB Serial commands for testing
 */
void sendTestRS485Command(uint8_t commandType, const uint8_t* data, uint8_t length) {
    // Create command buffer
    uint8_t buffer[RS485_MAX_COMMAND_LENGTH];
    uint8_t bufferIndex = 0;
    
    // Add start byte
    buffer[bufferIndex++] = 0xAA;
    
    // Add device ID (broadcast to all devices)
    buffer[bufferIndex++] = 0xFF;
    
    // Add command type
    buffer[bufferIndex++] = commandType;
    
    // Add data
    if (length > 0 && data != nullptr) {
        memcpy(&buffer[bufferIndex], data, length);
        bufferIndex += length;
    }
    
    // Add end byte
    buffer[bufferIndex++] = 0x55;
    
    // Send via RS-485
    sendRS485Response(0xFF, commandType, data, length);
    
    Serial.printf("Test command sent: Type=0x%02X, Length=%d\n", commandType, length);
}

/**
 * Handle USB Serial commands for testing
 */
void handleUSBSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command.startsWith("ping")) {
            // Send ping command via RS-485
            sendTestRS485Command(CMD_PING, nullptr, 0);
        }
        else if (command.startsWith("status")) {
            // Send status request via RS-485
            sendTestRS485Command(CMD_GET_STATUS, nullptr, 0);
        }
        else if (command.startsWith("voltage")) {
            // Set voltage via RS-485: voltage <value>
            // Example: voltage 5.0
            float voltage = command.substring(8).toFloat();
            if (voltage >= 0 && voltage <= 10) {
                uint16_t voltageRaw = (uint16_t)(voltage * 100);
                uint8_t data[2] = {(uint8_t)(voltageRaw >> 8), (uint8_t)(voltageRaw & 0xFF)};
                sendTestRS485Command(CMD_SET_VOLTAGE, data, 2);
            } else {
                Serial.println("Invalid voltage value (0-10V)");
            }
        }
        else if (command.startsWith("current")) {
            // Set current via RS-485: current <value>
            // Example: current 10.5
            float current = command.substring(8).toFloat();
            if (current >= 0 && current <= 25) {
                uint16_t currentRaw = (uint16_t)(current * 100);
                uint8_t data[2] = {(uint8_t)(currentRaw >> 8), (uint8_t)(currentRaw & 0xFF)};
                sendTestRS485Command(CMD_SET_CURRENT, data, 2);
            } else {
                Serial.println("Invalid current value (0-25mA)");
            }
        }
        else if (command.startsWith("sine")) {
            // Start sine wave via RS-485: sine <mode> <center> <amplitude> <period>
            // Example: sine v 5 2 1000 (voltage mode, center=5, amplitude=2, period=1000ms)
            int space1 = command.indexOf(' ', 5);
            int space2 = command.indexOf(' ', space1 + 1);
            int space3 = command.indexOf(' ', space2 + 1);
            int space4 = command.indexOf(' ', space3 + 1);
            
            if (space1 > 0 && space2 > 0 && space3 > 0 && space4 > 0) {
                char mode = command.charAt(space1 + 1);
                int center = command.substring(space2 + 1, space3).toInt();
                int amplitude = command.substring(space3 + 1, space4).toInt();
                int period = command.substring(space4 + 1).toInt();
                
                uint8_t modeByte;
                switch (mode) {
                    case 'v': modeByte = 0; break;
                    case 'c': modeByte = 1; break;
                    case 'd': modeByte = 2; break;
                    default:
                        Serial.println("Invalid mode (v/c/d)");
                        return;
                }
                
                uint8_t data[6] = {
                    modeByte,
                    (uint8_t)center,
                    (uint8_t)amplitude,
                    (uint8_t)(period >> 8),
                    (uint8_t)(period & 0xFF),
                    0x00 // Reserved
                };
                sendTestRS485Command(CMD_SINE_WAVE, data, 6);
            } else {
                Serial.println("Usage: sine <mode> <center> <amplitude> <period>");
            }
        }
        else if (command.startsWith("stop")) {
            // Stop sine wave via RS-485
            sendTestRS485Command(CMD_STOP_SINE, nullptr, 0);
        }
        else if (command.indexOf(',') > 0) {
            // New format: channel,mode,value
            // Example: 3,v,2.0 means channel 3 output 2.0V voltage
            int comma1 = command.indexOf(',');
            int comma2 = command.indexOf(',', comma1 + 1);
            
            if (comma1 > 0 && comma2 > 0) {
                int channel = command.substring(0, comma1).toInt();
                char mode = command.charAt(comma1 + 1);
                float value = command.substring(comma2 + 1).toFloat();
                
                if (channel >= 1 && channel <= 3) {
                    if (mode == 'v' || mode == 'c') {
                        // Set channel mode first
                        setRelayMode(channel, mode);
                        
                        // Then set value
                        if (mode == 'v') {
                            if (value >= 0 && value <= 10) {
                                setVoltageOutput(value);
                                Serial.printf("Channel %d set to VOLTAGE mode, output %.2fV\n", channel, value);
                            } else {
                                Serial.println("Invalid voltage value (0-10V)");
                            }
                        } else if (mode == 'c') {
                            if (value >= 0 && value <= 25) {
                                setCurrentOutput(value);
                                Serial.printf("Channel %d set to CURRENT mode, output %.2fmA\n", channel, value);
                            } else {
                                Serial.println("Invalid current value (0-25mA)");
                            }
                        }
                    } else {
                        Serial.println("Invalid mode (v/c)");
                    }
                } else {
                    Serial.println("Invalid channel (1-3)");
                }
            } else {
                Serial.println("Usage: channel,mode,value (e.g., 3,v,2.0)");
            }
        }
        else if (command.startsWith("help")) {
            printHelp();
        }
        else if (command.length() > 0) {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
}

/**
 * Print help information
 */
void printHelp() {
    Serial.println("\n=== USB Serial Commands ===");
    Serial.println("channel,mode,value      - Set channel output");
    Serial.println("  Example: 3,v,2.0      - Channel 3 output 2.0V voltage");
    Serial.println("  Example: 2,c,10.5     - Channel 2 output 10.5mA current");
    Serial.println("  channel: 1-3, mode: v(voltage)/c(current)");
    Serial.println("  voltage: 0-10V, current: 0-25mA");
    Serial.println("");
    Serial.println("ping                    - Send ping command via RS-485");
    Serial.println("status                  - Request status via RS-485");
    Serial.println("voltage <value>         - Set voltage output (0-10V)");
    Serial.println("current <value>         - Set current output (0-25mA)");
    Serial.println("sine <mode> <c> <a> <p> - Start sine wave");
    Serial.println("stop                    - Stop sine wave");
    Serial.println("help                    - Show this help");
    Serial.println("========================================\n");
} 