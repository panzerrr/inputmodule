#include "modbus_handler.h"
#include "dac_controller.h"
#include "sine_wave_generator.h"

// Global variables
uint16_t regAddresses[numRegisters]; 
bool dataReady[numRegisters] = {false}; 
uint64_t u64Values[numRegisters] = {0}; 
float floatValues[numRegisters] = {0}; 
int16_t int16Values[numRegisters] = {0}; 
char regTypes[numRegisters] = {0}; 

// Modbus instance
ModbusRTU mb;

// Configuration status
bool configDone = false;

uint16_t lowWord(uint32_t dword) {
    return (uint16_t)(dword & 0xFFFF);
}

uint16_t highWord(uint32_t dword) {
    return (uint16_t)(dword >> 16);
}

void initModbus() {
    Serial2.begin(BAUDRATE, PARITY, MODBUS_RX_PIN, MODBUS_TX_PIN);
    mb.begin(&Serial2, TXEN_PIN);
    mb.slave(SLAVE_ID);
    Serial.println("Modbus slave initialized on GPIO 16/17");
}

void processInput(String input) {
    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);
    int thirdComma = input.indexOf(',', secondComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
        Serial.println("Invalid command format.");
        return;
    }

    String regIndexStr = input.substring(0, firstComma);
    int regIndex = regIndexStr.toInt();

    if (regIndex < 0 || regIndex >= numRegisters) {
        Serial.println("Invalid register index.");
        return;
    }

    String regAddressStr = input.substring(firstComma + 1, secondComma);
    uint16_t regAddress = regAddressStr.toInt();

    char type = input.charAt(secondComma + 1);
    String valueStr = input.substring(thirdComma + 1);
    Serial.print("Received Command for Reg: ");
    Serial.print(regIndex);
    Serial.print(", Address: ");
    Serial.print(regAddress);
    Serial.print(", Type: ");
    Serial.print(type);
    Serial.print(", Value: ");
    Serial.println(valueStr);

    regAddresses[regIndex] = regAddress;
    regTypes[regIndex] = type;

    switch (type) {
        case 'I':
        case 'i':
            u64Values[regIndex] = (uint64_t)valueStr.toInt();
            break;
        case 'F':
        case 'f':
            floatValues[regIndex] = valueStr.toFloat();
            break;
        case 'S':
        case 's':
            int16Values[regIndex] = (int16_t)valueStr.toInt();
            break;
        default:
            Serial.println("Invalid type. Use I, F, or S (case-insensitive).");
            return;
    }
    dataReady[regIndex] = true;

    // Check if all data is ready and update the registers
    bool allReady = true;
    for (int i = 0; i < numRegisters; i++) {
        if (!dataReady[i]) {
            allReady = false;
            break;
        }
    }

    uint32_t asInt = 0; // Initialize outside of the switch scope

    if (allReady) {
        for (int i = 0; i < numRegisters; i++) {
            switch (regTypes[i]) {
                case 'I':
                case 'i':
                    mb.addHreg(regAddresses[i], 0x01, 2);
                    mb.Hreg(regAddresses[i], highWord(u64Values[i]));
                    mb.Hreg(regAddresses[i] + 1, lowWord(u64Values[i]));
                    break;
                case 'F':
                case 'f':
                    asInt = *(uint32_t*)&floatValues[i];
                    mb.addHreg(regAddresses[i], 0x01, 2);
                    mb.Hreg(regAddresses[i], highWord(asInt));
                    mb.Hreg(regAddresses[i] + 1, lowWord(asInt));
                    break;
                case 'S':
                case 's':
                    mb.addHreg(regAddresses[i], 0x01, 1);
                    mb.Hreg(regAddresses[i], (uint16_t)int16Values[i]);
                    break;
            }
            dataReady[i] = false; // Reset after processing
        }
        configDone = true;
        Serial.println("Modbus configuration completed - All registers updated");
        Serial.println("All analog outputs set to 0 due to Modbus activation");
        
        // Set all DAC outputs to 0 when Modbus is activated
        setAllDACsToZero();
    }
}

// Commented out functions as per original code
// void processU64(uint16_t regn, uint64_t data) {
//   mb.addHreg(regn,0x01,2);   //  
//   mb.Hreg(regn, highWord(data));
//   mb.Hreg(regn + 1, lowWord(data));
// }

// void processFloat(uint16_t regn, float data) {
//   uint32_t asInt = *(uint32_t*)&data;
//   mb.addHreg(regn,0x01,2);
//   mb.Hreg(regn, highWord(asInt));
//   mb.Hreg(regn + 1, lowWord(asInt));
// }

// void processInt16(uint16_t regn, int16_t data) {
//   mb.Hreg(regn, (uint16_t)data);
// }

/**
 * Set all DAC outputs to zero when Modbus is activated
 */
void setAllDACsToZero() {
    // Stop all sine wave generation
    stopSineWave(0); // Stop all channels
    
    // Set all voltage DACs to 0
    gp8413_1.setVoltage(0.0, 0); // SIG1 voltage channel
    gp8413_1.setVoltage(0.0, 1); // SIG2 voltage channel
    gp8413_2.setVoltage(0.0, 0); // SIG3 voltage channel
    
    // Set all current DACs to 0
    gp8313_1.setDACOutElectricCurrent(0); // SIG1 current channel
    gp8313_2.setDACOutElectricCurrent(0); // SIG2 current channel
    gp8313_3.setDACOutElectricCurrent(0); // SIG3 current channel
    
    Serial.println("All DAC outputs set to 0V/0mA");
}
