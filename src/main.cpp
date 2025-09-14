#include <Arduino.h>
#include <Wire.h>
#include "rs485_serial.h"
#include "rs485_command_handler.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "sine_wave_generator.h"
#include "device_id.h"
#include "modbus_handler.h"
#include "utils.h"

char signalModes[3] = {'v', 'v', 'v'};
float signalValues[3] = {0.0f, 0.0f, 0.0f}; // Track values for each signal
// Timing variables
unsigned long lastStatusReport = 0;
const unsigned long STATUS_REPORT_INTERVAL = 5000; // 5 seconds

// Forward declarations
void printStatusReport();
void printHelp();
void handleUSBSerialCommands();
void sendTestRS485Command(uint8_t commandType, const uint8_t* data, uint8_t length);
void testRS485Connection();

void setup() {
    // Initialize USB Serial for debugging
    Serial.begin(115200);
    Serial.println("=== ESP32 Input Module with RS-485 ===");
    
    // Initialize I2C communication
    Wire.begin(21, 22);  // SDA = GPIO21, SCL = GPIO22 (according to schematic)
    Serial.println("I2C initialized (SDA=GPIO21, SCL=GPIO22)");
    
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
    
    // Initialize RS-485 serial communication (暂时禁用第一路RS-485)
    // initRS485Serial();
    
    // Initialize RS-485 command handler (暂时禁用第一路RS-485)
    // initRS485CommandHandler();
    
    // Initialize Modbus slave
    initModbus();
    
    Serial.println("System initialization complete");
    Serial.println("USB Serial: Debug output only");
    Serial.println("RS-485 Serial: DISABLED (GPIO 19=TX, 18=RX) - 功能待定义");
    Serial.println("Modbus Slave: Interface (GPIO 17=TX, 16=RX)");
    Serial.println("Ready to receive commands...");
}

void loop() {
    // Process USB Serial commands
    handleUSBSerialCommands();
    
    // Process RS-485 commands (暂时禁用第一路RS-485)
    // if (handleRS485Commands()) {
    //     // Command was processed, no need to do anything else
    //     // The command handler will send responses automatically
    // }
    
    // Handle Modbus slave tasks
    mb.task();
    
    // Debug: Check for incoming Modbus data
    static unsigned long lastModbusDebug = 0;
    if (millis() - lastModbusDebug >= 10000) { // Every 10 seconds
        if (Serial2.available()) {
            Serial.printf("Modbus Serial2 available bytes: %d\n", Serial2.available());
        }
        lastModbusDebug = millis();
    }
    
    // Update sine wave generator
    updateSineWave();
    
    // Periodic status report disabled - use 'status' command instead
    // if (millis() - lastStatusReport >= STATUS_REPORT_INTERVAL) {
    //     printStatusReport();
    //     lastStatusReport = millis();
    // }
    
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
    
    // Modbus status
    if (configDone) {
        Serial.println("Modbus: ACTIVE (All analog outputs disabled)");
    } else {
        Serial.println("Modbus: INACTIVE");
    }
    
    // Signal status
    for (int i = 0; i < 3; i++) {
        if (configDone) {
            // When Modbus is active, show that outputs are disabled
            Serial.printf("SIG%d: DISABLED (Modbus active)\n", i + 1);
        } else {
            // Check if this channel is running sine wave
            if (isSineWaveActiveOnChannel(i)) {
                // Display sine wave parameters instead of current values
                float amplitude, period, center;
                char mode;
                if (getSineWaveParams(i, &amplitude, &period, &center, &mode)) {
                    const char* modeStr = (mode == 'v') ? "voltage" : "current";
                    const char* unit = (mode == 'v') ? "V" : "mA";
                    Serial.printf("SIG%d: %s mode, SINE WAVE (%.2f%s amplitude, %.1fs period, center %.2f%s)\n", 
                                 i + 1, modeStr, amplitude, unit, period, center, unit);
                }
            } else {
                // Display normal manual mode values
                char mode = signalModes[i];
                const char* modeStr = (mode == 'v') ? "voltage" : (mode == 'c') ? "current" : "unknown";
                
                if (mode == 'v') {
                    Serial.printf("SIG%d: %s mode, %.2f V\n", i + 1, modeStr, signalValues[i]);
                } else if (mode == 'c') {
                    Serial.printf("SIG%d: %s mode, %.2f mA\n", i + 1, modeStr, signalValues[i]);
                } else {
                    Serial.printf("SIG%d: %s mode\n", i + 1, modeStr);
                }
            }
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
        
        // Convert to lowercase for most commands, but keep modbus commands case-sensitive
        String lowerCommand = command;
        lowerCommand.toLowerCase();
        
        // Check if Modbus is active and block certain commands
        if (configDone && !command.startsWith("modbus") && !lowerCommand.startsWith("help") && !lowerCommand.startsWith("status")) {
            Serial.println("Command blocked: Modbus is active. Use 'modbus' commands or 'help' for options.");
            return;
        }
        
        if (lowerCommand.startsWith("ping")) {
            // Send ping command via RS-485 (暂时禁用第一路RS-485)
            Serial.println("RS-485功能暂时禁用，等待功能定义");
        }
        else if (lowerCommand.startsWith("test485")) {
            // Test RS-485 connection (暂时禁用第一路RS-485)
            Serial.println("RS-485功能暂时禁用，等待功能定义");
        }
        else if (lowerCommand.startsWith("status")) {
            // Show local system status
            printStatusReport();
        }
        else if (lowerCommand.startsWith("voltage")) {
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
        else if (lowerCommand.startsWith("current")) {
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
        else if (lowerCommand.startsWith("sine")) {
            // Handle sine wave commands directly
            parseSineWaveCommand(command);
        }
        else if (lowerCommand.startsWith("stop")) {
            // Stop sine wave via RS-485
            sendTestRS485Command(CMD_STOP_SINE, nullptr, 0);
        }
        else if (command.startsWith("modbus")) {
            // Modbus configuration command: modbus <reg_index>,<address>,<type>,<value>
            // Example: modbus 0,1000,I,12345
            String modbusCmd = command.substring(7); // Remove "modbus " prefix
            processInput(modbusCmd);
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
                        // Update signal mode first
                        signalModes[channel - 1] = mode;
                        
                        // Set channel mode first
                        setRelayMode(channel, mode);
                        
                        // Then set value using signal mapping
                        if (mode == 'v') {
                            if (value >= 0 && value <= 10) {
                                // Use signal mapping to set correct DAC and channel
                                signalMap[channel - 1].voltageDAC->setVoltage(value, signalMap[channel - 1].voltageChannel);
                                signalValues[channel - 1] = value; // Update signal value
                                Serial.printf("Channel %d set to VOLTAGE mode, output %.2fV\n", channel, value);
                                // Trigger status report after successful voltage setting
                                printStatusReport();
                            } else {
                                Serial.println("Invalid voltage value (0-10V)");
                            }
                        } else if (mode == 'c') {
                            if (value >= 0 && value <= 25) {
                                // Use signal mapping to set correct DAC
                                // Convert mA to DAC data: Rset=2kΩ, 25mA = 32767 (15-bit), so 1mA = 1310.68
                                signalMap[channel - 1].currentDAC->setDACOutElectricCurrent(static_cast<uint16_t>(value * 1310.68));
                                signalValues[channel - 1] = value; // Update signal value
                                Serial.printf("Channel %d set to CURRENT mode, output %.2fmA\n", channel, value);
                                // Trigger status report after successful current setting
                                printStatusReport();
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
        else if (lowerCommand.startsWith("modbus_test")) {
            // Test Modbus connection
            Serial.println("=== Modbus Connection Test ===");
            Serial.printf("Serial2 RX Pin: GPIO%d\n", MODBUS_RX_PIN);
            Serial.printf("Serial2 TX Pin: GPIO%d\n", MODBUS_TX_PIN);
            Serial.printf("Baud Rate: %d\n", BAUDRATE);
            Serial.printf("Parity: 8E1\n");
            Serial.printf("Slave ID: %d\n", SLAVE_ID);
            Serial.printf("TXEN Pin: %d\n", TXEN_PIN);
            Serial.printf("Serial2 available bytes: %d\n", Serial2.available());
            Serial.println("Available registers:");
            Serial.println("  0x0000: 0x1234 (Test register 1)");
            Serial.println("  0x0001: 0x5678 (Test register 2)");
            Serial.println("  0x0002: 0x9ABC (Test register 3)");
            Serial.println("  0x0003: 0xDEF0 (Test register 4)");
            if (configDone) {
                Serial.println("  User-configured registers are also available");
            } else {
                Serial.println("  User registers not configured yet");
            }
            
            Serial.println("Listening for Modbus requests for 15 seconds...");
            Serial.println("ModbusPoll settings should be:");
            Serial.println("  - Slave ID: 1");
            Serial.println("  - Function: 03 (Read Holding Registers)");
            Serial.println("  - Address: 0");
            Serial.println("  - Quantity: 1");
            Serial.println("  - Baud: 19200, 8E1");
            Serial.println("  - COM Port: Select correct port");
            Serial.println("");
            Serial.println("Starting monitoring...");
            
            unsigned long startTime = millis();
            int requestCount = 0;
            int totalBytes = 0;
            int modbusResponses = 0;
            
            while (millis() - startTime < 15000) {
                // Check for incoming data
                if (Serial2.available()) {
                    int bytes = Serial2.available();
                    totalBytes += bytes;
                    requestCount++;
                    Serial.printf("[%lu] Received data #%d: %d bytes\n", millis() - startTime, requestCount, bytes);
                    
                    // Read and display the raw data
                    uint8_t buffer[64];
                    int readBytes = Serial2.readBytes(buffer, min(bytes, 64));
                    Serial.print("Raw data: ");
                    for (int i = 0; i < readBytes; i++) {
                        Serial.printf("0x%02X ", buffer[i]);
                    }
                    Serial.println();
                    
                    // Try to parse as Modbus request
                    if (readBytes >= 8) { // Minimum Modbus RTU frame size
                        Serial.printf("Possible Modbus request: Slave=0x%02X, Func=0x%02X\n", buffer[0], buffer[1]);
                    }
                }
                
                // Process Modbus tasks
                if (mb.task()) {
                    modbusResponses++;
                    Serial.printf("[%lu] Modbus response sent #%d\n", millis() - startTime, modbusResponses);
                }
                
                delay(10);
            }
            
            Serial.printf("Test complete. Received %d data packets, %d total bytes.\n", requestCount, totalBytes);
            Serial.printf("Sent %d Modbus responses.\n", modbusResponses);
            
            if (requestCount == 0) {
                Serial.println("No data received! Check:");
                Serial.println("  1. USB-Serial adapter connection");
                Serial.println("  2. COM port selection in ModbusPoll");
                Serial.println("  3. Baud rate settings (19200)");
                Serial.println("  4. USB-Serial adapter driver");
            } else if (modbusResponses == 0) {
                Serial.println("Data received but no Modbus responses sent!");
                Serial.println("Check Modbus protocol settings:");
                Serial.println("  - Slave ID must be 1");
                Serial.println("  - Function must be 03 (Read Holding Registers)");
                Serial.println("  - Address must be 0-3");
            }
            Serial.println("=============================");
        }
        else if (lowerCommand.startsWith("serial_test")) {
            // Simple Serial2 loopback test
            Serial.println("=== Serial2 Loopback Test ===");
            Serial.println("This test will send data via Serial2 TX and read it back via RX");
            Serial.println("Connect GPIO 16 (RX) to GPIO 17 (TX) with a jumper wire");
            Serial.println("Starting test in 3 seconds...");
            delay(3000);
            
            const char* testMessage = "ESP32 Serial2 Test Message";
            Serial.printf("Sending: %s\n", testMessage);
            Serial2.write(testMessage);
            Serial2.write('\n');
            
            delay(100); // Wait for data to be sent
            
            Serial.println("Reading back data...");
            String received = "";
            unsigned long startTime = millis();
            while (millis() - startTime < 2000) {
                if (Serial2.available()) {
                    received += (char)Serial2.read();
                }
                delay(10);
            }
            
            if (received.length() > 0) {
                Serial.printf("Received: %s\n", received.c_str());
                Serial.println("Loopback test PASSED - Serial2 is working!");
            } else {
                Serial.println("Loopback test FAILED - No data received");
                Serial.println("Check jumper wire connection between GPIO 16 and 17");
            }
            Serial.println("=============================");
        }
        else if (lowerCommand.startsWith("send_modbus")) {
            // Send a test Modbus request
            Serial.println("=== Send Test Modbus Request ===");
            Serial.println("Sending test Modbus request to read register 0x0000");
            
            // Create a simple Modbus RTU request: [Slave ID][Function][Address High][Address Low][Quantity High][Quantity Low][CRC Low][CRC High]
            uint8_t request[] = {
                0x01,  // Slave ID
                0x03,  // Function 03 (Read Holding Registers)
                0x00,  // Address High
                0x00,  // Address Low (register 0)
                0x00,  // Quantity High
                0x01,  // Quantity Low (1 register)
                0x84,  // CRC Low (calculated for this request)
                0x0A   // CRC High
            };
            
            Serial.print("Sending request: ");
            for (int i = 0; i < 8; i++) {
                Serial.printf("0x%02X ", request[i]);
            }
            Serial.println();
            
            Serial2.write(request, 8);
            Serial2.flush();
            Serial.println("Request sent via Serial2");
            
            // Wait for response
            Serial.println("Waiting for response...");
            unsigned long startTime = millis();
            String response = "";
            while (millis() - startTime < 2000) {
                if (Serial2.available()) {
                    response += (char)Serial2.read();
                }
                delay(10);
            }
            
            if (response.length() > 0) {
                Serial.printf("Received response (%d bytes): ", response.length());
                for (int i = 0; i < response.length(); i++) {
                    Serial.printf("0x%02X ", (uint8_t)response[i]);
                }
                Serial.println();
            } else {
                Serial.println("No response received");
            }
            Serial.println("=============================");
        }
        else if (lowerCommand.startsWith("help")) {
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
    
    if (configDone) {
        Serial.println("NOTE: Modbus is ACTIVE - Analog outputs are disabled");
        Serial.println("Only Modbus and system commands are available");
        Serial.println("");
    } else {
        Serial.println("channel,mode,value      - Set channel output");
        Serial.println("  Example: 3,v,2.0      - Channel 3 output 2.0V voltage");
        Serial.println("  Example: 2,c,10.5     - Channel 2 output 10.5mA current");
        Serial.println("  channel: 1-3, mode: v(voltage)/c(current)");
        Serial.println("  voltage: 0-10V, current: 0-25mA");
        Serial.println("");
        Serial.println("SINE START <amp> <period> <center> <signal> <mode> - Start sine wave");
        Serial.println("  Example: SINE START 2.0 2.0 5.0 1 V");
        Serial.println("SINE STOP [signal]      - Stop sine wave");
        Serial.println("SINE STATUS             - Show sine wave status");
        Serial.println("");
    }
    
    Serial.println("ping                    - Send ping command via RS-485 (暂时禁用)");
    Serial.println("test485                 - Test RS-485 connection (暂时禁用)");
    Serial.println("status                  - Show local system status");
    Serial.println("modbus <reg>,<addr>,<type>,<value> - Configure Modbus register");
    Serial.println("  Example: modbus 0,1000,I,12345   - Set register 0 to address 1000, type I, value 12345");
    Serial.println("  Types: I(U64), F(Float), S(Int16)");
    Serial.println("  Note: Default test registers 0x0000-0x0003 are always available");
    Serial.println("  User registers are added when all 4 are configured");
    Serial.println("modbus_test             - Test Modbus connection and show configuration");
    Serial.println("serial_test             - Test Serial2 loopback (connect GPIO 16 to 17)");
    Serial.println("send_modbus             - Send test Modbus request");
    Serial.println("help                    - Show this help");
    Serial.println("========================================\n");
}

/**
 * Test RS-485 connection
 */
void testRS485Connection() {
    Serial.println("\n=== RS-485 Connection Test ===");
    
    // Test 1: Send ping command
    Serial.println("Test 1: Sending ping command...");
    sendTestRS485Command(CMD_PING, nullptr, 0);
    delay(100);
    
    // Test 2: Send voltage command
    Serial.println("Test 2: Sending voltage command (5.0V)...");
    uint16_t voltageRaw = (uint16_t)(5.0 * 100);
    uint8_t data[2] = {(uint8_t)(voltageRaw >> 8), (uint8_t)(voltageRaw & 0xFF)};
    sendTestRS485Command(CMD_SET_VOLTAGE, data, 2);
    delay(100);
    
    // Test 3: Send current command
    Serial.println("Test 3: Sending current command (10.0mA)...");
    uint16_t currentRaw = (uint16_t)(10.0 * 100);
    uint8_t currentData[2] = {(uint8_t)(currentRaw >> 8), (uint8_t)(currentRaw & 0xFF)};
    sendTestRS485Command(CMD_SET_CURRENT, currentData, 2);
    delay(100);
    
    // Test 4: Check for incoming data
    Serial.println("Test 4: Checking for incoming RS-485 data...");
    Serial.println("Listening for 2 seconds...");
    
    unsigned long startTime = millis();
    int bytesReceived = 0;
    
    while (millis() - startTime < 2000) {
        if (processRS485Commands()) {
            bytesReceived++;
            Serial.printf("Received command #%d\n", bytesReceived);
        }
        delay(10);
    }
    
    if (bytesReceived == 0) {
        Serial.println("No RS-485 data received during test period");
        Serial.println("Check wiring: TX=GPIO19, RX=GPIO18");
        Serial.println("Baud rate: 19200, Parity: 8E1");
    } else {
        Serial.printf("Successfully received %d commands\n", bytesReceived);
    }
    
    Serial.println("=== RS-485 Test Complete ===\n");
} 