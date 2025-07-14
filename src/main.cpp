#include <Arduino.h>
#include "rs485_serial.h"
#include "rs485_command_handler.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "sine_wave_generator.h"
#include "device_id.h"
#include "modbus_handler.h"

char signalModes[3] = {'v', 'v', 'v'};
// Timing variables
unsigned long lastStatusReport = 0;
const unsigned long STATUS_REPORT_INTERVAL = 5000; // 5 seconds

// 新增：每通道模式和数值
char channelModes[3] = {'v', 'v', 'v'}; // 'v' or 'c'
float channelValues[3] = {0.0f, 0.0f, 0.0f}; // 输出值

// Forward declarations
void printStatusReport();
void printHelp();
void handleUSBSerialCommands();
void sendTestRS485Command(uint8_t commandType, const uint8_t* data, uint8_t length);
void setChannelOutput(uint8_t channel, char mode, float value);

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

    // Default to three-channel voltage mode on startup
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
    
    // Initialize Modbus slave
    initModbus();
    
    Serial.println("System initialization complete");
    Serial.println("USB Serial: Debug output only");
    Serial.println("RS-485 Serial: Command interface (GPIO 19=TX, 18=RX, 21=DE)");
    Serial.println("Modbus Slave: Interface (GPIO 17=TX, 16=RX)");
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
    
    // Handle Modbus slave tasks
    mb.task();
    
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
    
    // 修改 printStatusReport，循环显示每个通道
    Serial.println("\n=== Channel Status ===");
    for (int i = 0; i < 3; ++i) {
        if (channelModes[i] == 'v') {
            Serial.printf("Channel %d: Voltage mode, %.2fV\n", i+1, channelValues[i]);
        } else if (channelModes[i] == 'c') {
            Serial.printf("Channel %d: Current mode, %.2fmA\n", i+1, channelValues[i]);
        } else {
            Serial.printf("Channel %d: Unknown mode\n", i+1);
        }
    }
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
        // 不再整体toLowerCase，保留参数原样
        String cmdLower = command;
        cmdLower.toLowerCase();
        if (cmdLower.startsWith("ping")) {
            sendTestRS485Command(CMD_PING, nullptr, 0);
        }
        else if (cmdLower.startsWith("status")) {
            sendTestRS485Command(CMD_GET_STATUS, nullptr, 0);
        }
        else if (cmdLower.startsWith("voltage")) {
            float voltage = command.substring(8).toFloat();
            if (voltage >= 0 && voltage <= 10) {
                uint16_t voltageRaw = (uint16_t)(voltage * 100);
                uint8_t data[2] = {(uint8_t)(voltageRaw >> 8), (uint8_t)(voltageRaw & 0xFF)};
                sendTestRS485Command(CMD_SET_VOLTAGE, data, 2);
            } else {
                Serial.println("Invalid voltage value (0-10V)");
            }
        }
        else if (cmdLower.startsWith("current")) {
            float current = command.substring(8).toFloat();
            if (current >= 0 && current <= 25) {
                uint16_t currentRaw = (uint16_t)(current * 100);
                uint8_t data[2] = {(uint8_t)(currentRaw >> 8), (uint8_t)(currentRaw & 0xFF)};
                sendTestRS485Command(CMD_SET_CURRENT, data, 2);
            } else {
                Serial.println("Invalid current value (0-25mA)");
            }
        }
        else if (cmdLower.startsWith("sine")) {
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
                    case 'v': case 'V': modeByte = 0; break;
                    case 'c': case 'C': modeByte = 1; break;
                    case 'd': case 'D': modeByte = 2; break;
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
        else if (cmdLower.startsWith("stop")) {
            sendTestRS485Command(CMD_STOP_SINE, nullptr, 0);
        }
        else if (cmdLower.startsWith("modbus")) {
            String modbusCmd = command.substring(7); // Remove "modbus " prefix
            processInput(modbusCmd);
        }
        else if (command.indexOf(',') > 0) {
            int comma1 = command.indexOf(',');
            int comma2 = command.indexOf(',', comma1 + 1);
            if (comma1 > 0 && comma2 > 0) {
                int channel = command.substring(0, comma1).toInt();
                char mode = command.charAt(comma1 + 1);
                float value = command.substring(comma2 + 1).toFloat();
                if (channel >= 1 && channel <= 3) {
                    if (mode == 'v' || mode == 'c' || mode == 'V' || mode == 'C') {
                        setChannelOutput(channel, tolower(mode), value);
                        Serial.printf("Channel %d set to %s mode, output %.2f%s\n", channel, (mode == 'v' || mode == 'V') ? "VOLTAGE" : "CURRENT", value, (mode == 'v' || mode == 'V') ? "V" : "mA");
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
        else if (cmdLower.startsWith("help")) {
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
    Serial.println("modbus <reg>,<addr>,<type>,<value> - Configure Modbus register");
    Serial.println("  Example: modbus 0,1000,I,12345   - Set register 0 to address 1000, type I, value 12345");
    Serial.println("  Types: I(U64), F(Float), S(Int16)");
    Serial.println("help                    - Show this help");
    Serial.println("========================================\n");
} 

void setChannelOutput(uint8_t channel, char mode, float value) {
    if (channel < 1 || channel > 3) return;
    channelModes[channel - 1] = mode;
    channelValues[channel - 1] = value;
    setRelayMode(channel, mode);
    if (mode == 'v') {
        setVoltageOutput(value);
    } else if (mode == 'c') {
        setCurrentOutput(value);
    }
} 