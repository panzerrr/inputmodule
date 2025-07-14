#ifndef MODBUSSLAVE_H
#define MODBUSSLAVE_H

#include <Arduino.h>
#include <ModbusRTU.h>

// Define ModbusSlave class
class ModbusSlave {
public:
    /**
     * @brief Constructor
     * @param modbus ModbusRTU instance reference
     */
    ModbusSlave(ModbusRTU &modbus);

    /**
     * @brief Initialize Modbus slave
     * @param slaveID Slave address
     * @param baudrate Baud rate
     * @param parity Serial parameters
     * @param txPin (Optional) TX enable pin for RS485, default -1
     */
    void begin(uint8_t slaveID, uint32_t baudrate, uint8_t parity, int8_t txPin = -1);

    /**
     * @brief Process input commands
     * @param input Input command string
     */
    void processInput(const String &input);

    /**
     * @brief Execute Modbus tasks
     */
    void handleModbus();

private:
    ModbusRTU &mb;               // Modbus slave instance
    static const int numRegisters = 4; // Number of available registers
    uint16_t regAddresses[numRegisters]; // Register addresses
    bool dataReady[numRegisters];       // Whether data is ready
    uint64_t u64Values[numRegisters];   // Store U64 type data
    float floatValues[numRegisters];    // Store Float type data
    int16_t int16Values[numRegisters];  // Store Int16 type data
    char regTypes[numRegisters];        // Register types: 'I' (U64), 'F' (Float), 'S' (Int16)

    /**
     * @brief Update Modbus registers
     */
    void updateRegisters();
};

#endif // MODBUSSLAVE_H
