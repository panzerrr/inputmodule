#ifndef UART_COMMAND_H
#define UART_COMMAND_H

#include <Arduino.h>
#include <HardwareSerial.h>

class UARTCommand {
public:
    UARTCommand(HardwareSerial &serial, uint8_t nodeId);

    void begin(uint32_t baudRate);

    void process();

private:
    HardwareSerial &serial; // UART reference
    uint8_t nodeId;         // Current slave address
    uint8_t buffer[64];     // Receive buffer

    void parseCommand(const uint8_t *data, size_t length);

    void executeRead(uint16_t regAddress);

    void sendResponse(uint8_t command, uint16_t regAddress, uint16_t value);

    uint8_t calculateChecksum(const uint8_t *data, size_t length);
};

#endif // UART_COMMAND_H