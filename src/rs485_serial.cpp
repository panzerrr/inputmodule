#include "rs485_serial.h"
#include "device_id.h"

void handleUSBSerialCommands(String command);

// Global variables
static uint8_t currentDeviceID = 0;
static uint8_t rs485Buffer[RS485_BUFFER_SIZE];
static uint8_t bufferIndex = 0;
static RS485Command lastCommand;

// 新增缓冲区
static char rs485LineBuffer[128];
static uint8_t rs485LineIndex = 0;

// Forward declaration
bool processCommand();

// HardwareSerial instance for RS-485 interface
HardwareSerial RS485Serial(RS485_SERIAL_NUM);  // Serial1 for work mode (receiving external RS-485 signals)

/**
 * Initialize RS-485 serial communication
 */
void initRS485Serial() {
    // Initialize work mode RS-485 serial (receiving external RS-485 signals)
    RS485Serial.begin(RS485_BAUDRATE, RS485_PARITY, RS485_RX_PIN, RS485_TX_PIN);
    
    // Get device ID from hardware jumpers
    initDeviceIDPins();
    currentDeviceID = calculateDeviceID();
    
    // Clear buffers
    memset(rs485Buffer, 0, RS485_BUFFER_SIZE);
    bufferIndex = 0;
    lastCommand.valid = false;
    
    Serial.printf("Work Mode RS-485: GPIO %d(TX), %d(RX)\n", RS485_TX_PIN, RS485_RX_PIN);
    Serial.printf("Device ID: %d, Baud Rate: %d\n", currentDeviceID, RS485_BAUDRATE);
}

/**
 * Process incoming RS-485 commands from work mode interface
 * @return true if a valid command was received
 */
bool processRS485Commands() {
    while (RS485Serial.available()) {
        char c = RS485Serial.read();
        if (c == '\n' || rs485LineIndex >= sizeof(rs485LineBuffer) - 1) {
            rs485LineBuffer[rs485LineIndex] = '\0';
            if (rs485LineIndex > 0) {
                // 直接调用主命令解析函数
                handleUSBSerialCommands(String(rs485LineBuffer));
            }
            rs485LineIndex = 0;
        } else if (c != '\r') {
            rs485LineBuffer[rs485LineIndex++] = c;
        }
    }
    return false;
}

/**
 * Process a complete command from buffer
 * @return true if command is valid and for this device
 */
bool processCommand() {
    if (bufferIndex < 4) return false; // Minimum command length
    
    // Check start and end bytes
    if (rs485Buffer[0] != 0xAA || rs485Buffer[bufferIndex - 1] != 0x55) {
        return false;
    }
    
    // Extract device ID and command type
    uint8_t targetDeviceID = rs485Buffer[1];
    uint8_t commandType = rs485Buffer[2];
    
    // Check if command is for this device (0xFF = broadcast)
    if (targetDeviceID != currentDeviceID && targetDeviceID != 0xFF) {
        return false;
    }
    
    // Store command
    lastCommand.deviceID = targetDeviceID;
    lastCommand.commandType = commandType;
    lastCommand.length = bufferIndex - 4; // Exclude start, device ID, command, end
    
    // Copy data
    if (lastCommand.length > 0 && lastCommand.length < RS485_MAX_COMMAND_LENGTH - 2) {
        memcpy(lastCommand.data, &rs485Buffer[3], lastCommand.length);
    }
    
    lastCommand.valid = true;
    
    // Log received command
    Serial.printf("Work Mode RS-485 Command received: Device=%d, Type=0x%02X, Length=%d\n", targetDeviceID, commandType, lastCommand.length);
    
    return true;
}

/**
 * Send response via RS-485 work mode interface
 * @param deviceID Target device ID
 * @param commandType Command type
 * @param data Response data
 * @param length Data length
 */
void sendRS485Response(uint8_t deviceID, uint8_t commandType, const uint8_t* data, uint8_t length) {
    if (length > RS485_MAX_COMMAND_LENGTH - 2) {
        Serial.println("RS-485: Response too long");
        return;
    }
    // Send response: [START][DEVICE_ID][COMMAND][DATA...][END]
    RS485Serial.write(0xAA); // Start byte
    RS485Serial.write(deviceID);
    RS485Serial.write(commandType);
    if (length > 0 && data != nullptr) {
        RS485Serial.write(data, length);
    }
    RS485Serial.write(0x55); // End byte
    RS485Serial.flush(); // Ensure all data is sent
    Serial.printf("Work Mode RS-485 Response sent: Device=%d, Type=0x%02X, Length=%d\n", deviceID, commandType, length);
}

/**
 * Get the last received command
 * @return Pointer to the last received command
 */
RS485Command* getLastCommand() {
    return &lastCommand;
}

/**
 * Check if RS-485 is available (work mode interface)
 * @return true if data is available
 */
bool isRS485Available() {
    return RS485Serial.available() > 0;
}

/**
 * Set device ID for filtering commands
 * @param id Device ID to filter for
 */
void setDeviceID(uint8_t id) {
    currentDeviceID = id;
    Serial.printf("RS-485 Device ID set to: %d\n", id);
}

/**
 * Get current device ID
 * @return Current device ID
 */
uint8_t getCurrentDeviceID() {
    return currentDeviceID;
}

/**
 * Send acknowledgment response
 * @param success true for success, false for error
 */
void sendAckResponse(bool success) {
    uint8_t response = success ? 0x01 : 0x00;
    sendRS485Response(lastCommand.deviceID, lastCommand.commandType, &response, 1);
}

/**
 * Send data response
 * @param data Data to send
 * @param length Data length
 */
void sendDataResponse(const uint8_t* data, uint8_t length) {
    sendRS485Response(lastCommand.deviceID, lastCommand.commandType, data, length);
} 