#include "modbusslave.h"

// Constructor
ModbusSlave::ModbusSlave(ModbusRTU &modbus) : mb(modbus) {
    memset(regAddresses, 0, sizeof(regAddresses));
    memset(dataReady, 0, sizeof(dataReady));
    memset(u64Values, 0, sizeof(u64Values));
    memset(floatValues, 0, sizeof(floatValues));
    memset(int16Values, 0, sizeof(int16Values));
    memset(regTypes, 0, sizeof(regTypes));
}

// Initialize Modbus slave
void ModbusSlave::begin(uint8_t slaveID, uint32_t baudrate, uint8_t parity, int8_t txPin) {
    Serial1.begin(baudrate, parity);
    mb.begin(&Serial1, txPin);
    mb.slave(slaveID);

    Serial.println("Modbus slave initialized.");
    Serial.println("Send command in format: REG_INDEX,ADDRESS,TYPE,VALUE");
    Serial.println("Types: I - U64, F - Float, S - Int16");
}

// Process input commands
void ModbusSlave::processInput(const String &input) {
    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);
    int thirdComma = input.indexOf(',', secondComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
        Serial.println("Invalid command format.");
        return;
    }

    int regIndex = input.substring(0, firstComma).toInt();
    if (regIndex < 0 || regIndex >= numRegisters) {
        Serial.println("Invalid register index.");
        return;
    }

    uint16_t regAddress = input.substring(firstComma + 1, secondComma).toInt();
    char type = input.charAt(secondComma + 1);
    String valueStr = input.substring(thirdComma + 1);

    regAddresses[regIndex] = regAddress;
    regTypes[regIndex] = type;

    switch (type) {
        case 'I': // 64-bit integer
            u64Values[regIndex] = valueStr.toInt();
            break;
        case 'F': // Float type
            floatValues[regIndex] = valueStr.toFloat();
            break;
        case 'S': // 16-bit integer
            int16Values[regIndex] = valueStr.toInt();
            break;
        default:
            Serial.println("Invalid type. Use I, F, or S.");
            return;
    }
    dataReady[regIndex] = true;

    updateRegisters();
}

// Update Modbus registers
 
void ModbusSlave::updateRegisters() {
    for (int i = 0; i < numRegisters; i++) {
        if (dataReady[i]) {
            uint32_t asInt = 0; // Declare variable in advance
            switch (regTypes[i]) {
                case 'I': // 64-bit integer
                    mb.addHreg(regAddresses[i], 0x01, 2);
                    mb.Hreg(regAddresses[i], (uint16_t)(u64Values[i] >> 16));
                    mb.Hreg(regAddresses[i] + 1, (uint16_t)(u64Values[i] & 0xFFFF));
                    break;
                case 'F': // Float type
                    asInt = *(uint32_t *)&floatValues[i];
                    mb.addHreg(regAddresses[i], 0x01, 2);
                    mb.Hreg(regAddresses[i], (uint16_t)(asInt >> 16));
                    mb.Hreg(regAddresses[i] + 1, (uint16_t)(asInt & 0xFFFF));
                    break;
                case 'S': // 16-bit integer
                    mb.addHreg(regAddresses[i], 0x01, 1);
                    mb.Hreg(regAddresses[i], (uint16_t)int16Values[i]);
                    break;
            }
            dataReady[i] = false; // Data processed
        }
    }
    Serial.println("Registers updated.");
}
// Execute Modbus tasks
void ModbusSlave::handleModbus() {
    mb.task();
    delay(10);
}
