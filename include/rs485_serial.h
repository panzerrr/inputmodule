#ifndef RS485_SERIAL_H
#define RS485_SERIAL_H

#include <Arduino.h>
#include <HardwareSerial.h>

// RS-485 Serial Configuration - Work mode receiving interface (receiving external RS-485 signals)
#define RS485_SERIAL_NUM 1        // Use Serial1 for receiving external RS-485
#define RS485_TX_PIN 19           // GPIO 19 for TX (另一路RS-485)
#define RS485_RX_PIN 18           // GPIO 18 for RX (另一路RS-485)
#define RS485_BAUDRATE 19200      // Baud rate
#define RS485_PARITY SERIAL_8E1   // 8 data bits, Even parity, 1 stop bit

// Buffer sizes
#define RS485_BUFFER_SIZE 64      // Receive buffer size
#define RS485_MAX_COMMAND_LENGTH 32

// Command structure
struct RS485Command {
    uint8_t deviceID;             // Target device ID
    uint8_t commandType;          // Command type
    uint8_t data[RS485_MAX_COMMAND_LENGTH - 2]; // Command data
    uint8_t length;               // Total command length
    bool valid;                   // Command validity flag
};

/**
 * Initialize RS-485 serial communication
 */
void initRS485Serial();

/**
 * Process incoming RS-485 commands from work mode interface
 * @return true if a valid command was received
 */
bool processRS485Commands();

/**
 * Send response via RS-485 work mode interface
 * @param deviceID Target device ID
 * @param commandType Command type
 * @param data Response data
 * @param length Data length
 */
void sendRS485Response(uint8_t deviceID, uint8_t commandType, const uint8_t* data, uint8_t length);

/**
 * Get the last received command
 * @return Pointer to the last received command
 */
RS485Command* getLastCommand();

/**
 * Check if RS-485 is available
 * @return true if data is available
 */
bool isRS485Available();

/**
 * Set device ID for filtering commands
 * @param id Device ID to filter for
 */
void setDeviceID(uint8_t id);

/**
 * Get current device ID
 * @return Current device ID
 */
uint8_t getCurrentDeviceID();

/**
 * Send acknowledgment response via RS-485
 * @param success true for success, false for error
 */
void sendAckResponse(bool success);

/**
 * Send data response via RS-485
 * @param data Data to send
 * @param length Data length
 */
void sendDataResponse(const uint8_t* data, uint8_t length);

#endif // RS485_SERIAL_H 