#ifndef MODBUS_HANDLER_H
#define MODBUS_HANDLER_H

#include <ModbusRTU.h>

// Modbus configuration
#define SLAVE_ID 0x01       // Device Address but maybe in the future we can make variations?
#define BAUDRATE 19200      // Serial Bit Rate
#define PARITY SERIAL_8E1   // 8 data bits, Even parity, 1 stop bit
#define MODBUS_TX_PIN 17    // GPIO 17 for Modbus TX
#define MODBUS_RX_PIN 16    // GPIO 16 for Modbus RX
#define TXEN_PIN -1         // Not used in RS-232 or USB-Serial

// Maximum number of registers supported
const int numRegisters = 4;

// Data type enumeration
enum DataType {
    TYPE_U64,
    TYPE_FLOAT,
    TYPE_INT16
}; // Construct our data type, as I checked the excel only found these 3

// Global variables
extern uint16_t regAddresses[numRegisters];  // Register addresses
extern char regTypes[numRegisters];          // Data types
extern uint64_t u64Values[numRegisters];     // U64 data
extern float floatValues[numRegisters];      // Float data
extern int16_t int16Values[numRegisters];    // Int16 data
extern bool dataReady[numRegisters];         // Data ready status

// Modbus instance
extern ModbusRTU mb;

// Configuration status
extern bool configDone;

// Utility functions
uint16_t lowWord(uint32_t dword);
uint16_t highWord(uint32_t dword);

// Initialize Modbus
void initModbus();

// Process input command
void processInput(String input);

// Set all DAC outputs to zero when Modbus is activated
void setAllDACsToZero();

// Process different data types (commented out as per original code)
// void processU64(uint16_t regn, uint64_t data);
// void processFloat(uint16_t regn, float data);
// void processInt16(uint16_t regn, int16_t data);

#endif // MODBUS_HANDLER_H
