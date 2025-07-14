#include "uart_command.h"

UARTCommand::UARTCommand(HardwareSerial &serial, uint8_t nodeId)
    : serial(serial), nodeId(nodeId) {}

void UARTCommand::begin(uint32_t baudRate) {
    serial.begin(baudRate);
}

void UARTCommand::process() {
    if (serial.available()) {
        size_t length = serial.readBytes(buffer, sizeof(buffer));

        if (length > 4 && buffer[0] == 0xAA && buffer[1] == nodeId) {
            uint8_t checksum = calculateChecksum(buffer, length - 1);
            if (checksum == buffer[length - 1]) {
                parseCommand(buffer + 2, length - 3);
            }
        }
    }
}

void UARTCommand::parseCommand(const uint8_t *data, size_t length) {
    uint8_t command = data[0];
    uint16_t regAddress = (data[1] << 8) | data[2];

    switch (command) {
        case 0x01: // Read register
            executeRead(regAddress);
            break;
        case 0x02: // Write register (example, not implemented)
            break;
        default:
            break;
    }
}

void UARTCommand::executeRead(uint16_t regAddress) {
    uint16_t value = 0x1234; // Example value
    sendResponse(0x01, regAddress, value);
}

void UARTCommand::sendResponse(uint8_t command, uint16_t regAddress, uint16_t value) {
    uint8_t response[8];
    response[0] = 0xAA;               // Start byte
    response[1] = nodeId;             // Slave address
    response[2] = command;            // Command type
    response[3] = (regAddress >> 8);  // Register address high byte
    response[4] = (regAddress & 0xFF);// Register address low byte
    response[5] = (value >> 8);       // Data high byte
    response[6] = (value & 0xFF);     // Data low byte
    response[7] = calculateChecksum(response, 7);

    serial.write(response, sizeof(response)); // Send data
    serial.flush(); // Ensure data is sent
}

uint8_t UARTCommand::calculateChecksum(const uint8_t *data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return ~checksum + 1;
}