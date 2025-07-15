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

// Modbus正弦波模式全局变量
bool modbusSineActive = false;
int modbusSineRegIndex = -1;
float modbusSineAmplitude = 0;
float modbusSinePeriod = 1;
float modbusSineCenter = 0;
unsigned long modbusSineStartTime = 0;

// Forward declarations
void printStatusReport();
void printHelp();
void handleUSBSerialCommands();
void handleUSBSerialCommands(String command);
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
    
    // Modbus正弦波实时写入
    if (modbusSineActive && modbusSineRegIndex >= 0 && modbusSineRegIndex < numRegisters) {
        unsigned long elapsed = millis() - modbusSineStartTime;
        float t = (elapsed / 1000.0f);
        float value = modbusSineCenter + modbusSineAmplitude * sin(2 * PI * t / modbusSinePeriod);
        uint16_t regAddr = regAddresses[modbusSineRegIndex];
        char type = regTypes[modbusSineRegIndex];
        if (type == 'F' || type == 'f') {
            floatValues[modbusSineRegIndex] = value;
            uint32_t asInt = *(uint32_t*)&value;
            mb.Hreg(regAddr, (asInt >> 16) & 0xFFFF);
            mb.Hreg(regAddr + 1, asInt & 0xFFFF);
        } else if (type == 'I' || type == 'i') {
            uint64_t v = (uint64_t)value;
            mb.Hreg(regAddr, (v >> 48) & 0xFFFF);
            mb.Hreg(regAddr + 1, (v >> 32) & 0xFFFF);
            mb.Hreg(regAddr + 2, (v >> 16) & 0xFFFF);
            mb.Hreg(regAddr + 3, v & 0xFFFF);
        } else if (type == 'S' || type == 's') {
            int16Values[modbusSineRegIndex] = (int16_t)value;
            mb.Hreg(regAddr, (int16_t)value);
        }
    }
    
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
        handleUSBSerialCommands(command);
    }
}

/**
 * Handle USB Serial commands for testing
 */
void handleUSBSerialCommands(String command) {
    command.trim();
    if (command.length() == 0) return;
    
    // 使用更高效的字符串比较，避免toLowerCase
    if (command.startsWith("ping") || command.startsWith("PING")) {
        sendTestRS485Command(CMD_PING, nullptr, 0);
    }
    else if (command.startsWith("status") || command.startsWith("STATUS")) {
        sendTestRS485Command(CMD_GET_STATUS, nullptr, 0);
    }
    else if (command.startsWith("voltage") || command.startsWith("VOLTAGE")) {
        float voltage = command.substring(8).toFloat();
        if (voltage >= 0 && voltage <= 10) {
            uint16_t voltageRaw = (uint16_t)(voltage * 100);
            uint8_t data[2] = {(uint8_t)(voltageRaw >> 8), (uint8_t)(voltageRaw & 0xFF)};
            sendTestRS485Command(CMD_SET_VOLTAGE, data, 2);
        } else {
            Serial.println("Invalid voltage value (0-10V)");
        }
    }
    else if (command.startsWith("current") || command.startsWith("CURRENT")) {
        float current = command.substring(8).toFloat();
        if (current >= 0 && current <= 25) {
            uint16_t currentRaw = (uint16_t)(current * 100);
            uint8_t data[2] = {(uint8_t)(currentRaw >> 8), (uint8_t)(currentRaw & 0xFF)};
            sendTestRS485Command(CMD_SET_CURRENT, data, 2);
        } else {
            Serial.println("Invalid current value (0-25mA)");
        }
    }
    else if (command.startsWith("sine") || command.startsWith("SINE")) {
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
    else if (command.startsWith("stop") || command.startsWith("STOP")) {
        sendTestRS485Command(CMD_STOP_SINE, nullptr, 0);
    }
    else if (command.startsWith("modbus") || command.startsWith("MODBUS")) {
        String modbusCmd = command.substring(7); // Remove "modbus " prefix
        processInput(modbusCmd);
    }
    else if (command.startsWith("sine_modbus") || command.startsWith("SINE_MODBUS")) {
        // 例：sine_modbus 5.0 2.0 5.0 0
        //     幅值 周期(s) 中心 reg_index
        String params = command.substring(11);
        params.trim();
        int space1 = params.indexOf(' ');
        int space2 = params.indexOf(' ', space1 + 1);
        int space3 = params.indexOf(' ', space2 + 1);
        if (space1 > 0 && space2 > 0 && space3 > 0) {
            modbusSineAmplitude = params.substring(0, space1).toFloat();
            modbusSinePeriod = params.substring(space1 + 1, space2).toFloat();
            modbusSineCenter = params.substring(space2 + 1, space3).toFloat();
            modbusSineRegIndex = params.substring(space3 + 1).toInt();
            modbusSineActive = true;
            modbusSineStartTime = millis();
            Serial.println("Modbus Sinewave started.");
        } else {
            Serial.println("Usage: sine_modbus <amplitude> <period> <center> <reg_index>");
        }
    }
    else if (command.startsWith("stop_modbus_sine") || command.startsWith("STOP_MODBUS_SINE")) {
        modbusSineActive = false;
        Serial.println("Modbus Sinewave stopped.");
    }
    else if (command.indexOf(',') > 0) {
        int comma1 = command.indexOf(',');
        int comma2 = command.indexOf(',', comma1 + 1);
        if (comma1 > 0 && comma2 > 0) {
            int channel = command.substring(0, comma1).toInt();
            char mode = command.charAt(comma1 + 1);
            float value = command.substring(comma2 + 1).toFloat();
            if (channel >= 1 && channel <= 3) {
                // 使用更安全的字符比较
                char modeLower = (mode >= 'A' && mode <= 'Z') ? mode + 32 : mode;
                if (modeLower == 'v' || modeLower == 'c') {
                    setChannelOutput(channel, modeLower, value);
                    Serial.printf("Channel %d set to %s mode, output %.2f%s\n", 
                                channel, 
                                (modeLower == 'v') ? "VOLTAGE" : "CURRENT", 
                                value, 
                                (modeLower == 'v') ? "V" : "mA");
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
    else if (command.startsWith("help") || command.startsWith("HELP")) {
        printHelp();
    }
    else {
        Serial.println("Unknown command. Type 'help' for available commands.");
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
    Serial.println("  Example: modbus 0,1000,F,0   - Set register 0 to address 1000, type F, value 0");
    Serial.println("sine_modbus <ampl> <period> <center> <reg_index> - Output sinewave to modbus register");
    Serial.println("  Example: sine_modbus 5.0 2.0 5.0 0   - 5.0幅值, 2.0秒周期, 5.0中心, 写到第0号modbus寄存器");
    Serial.println("stop_modbus_sine              - Stop modbus sinewave output");
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