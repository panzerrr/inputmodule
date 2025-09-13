#include "rs485_command_handler.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "sine_wave_generator.h"
#include "device_id.h"

// Forward declaration
void printStatusReport();

/**
 * Initialize RS-485 command handler
 */
void initRS485CommandHandler() {
    Serial.println("RS-485 Command Handler initialized");
}

/**
 * Process RS-485 commands and execute corresponding actions
 * @return true if a command was processed
 */
bool handleRS485Commands() {
    if (processRS485Commands()) {
        RS485Command* command = getLastCommand();
        if (command && command->valid) {
            return executeRS485Command(command);
        }
    }
    return false;
}

/**
 * Execute a specific command
 * @param command Pointer to the command structure
 * @return true if command was executed successfully
 */
bool executeRS485Command(RS485Command* command) {
    if (!command || !command->valid) {
        return false;
    }
    
    bool success = false;
    
    switch (command->commandType) {
        case CMD_PING:
            success = handlePingCommand(command->data, command->length);
            break;
            
        case CMD_GET_DEVICE_ID:
            success = handleGetDeviceIDCommand(command->data, command->length);
            break;
            
        case CMD_SET_VOLTAGE:
            success = handleSetVoltageCommand(command->data, command->length);
            break;
            
        case CMD_SET_CURRENT:
            success = handleSetCurrentCommand(command->data, command->length);
            break;
            
        case CMD_SET_RELAY:
            success = handleSetRelayCommand(command->data, command->length);
            break;
            
        case CMD_GET_STATUS:
            success = handleGetStatusCommand(command->data, command->length);
            break;
            
        case CMD_SINE_WAVE:
            success = handleSineWaveCommand(command->data, command->length);
            break;
            
        case CMD_STOP_SINE:
            success = handleStopSineCommand(command->data, command->length);
            break;
            
        default:
            Serial.printf("Unknown command: 0x%02X\n", command->commandType);
            sendAckResponse(false);
            return false;
    }
    
    sendAckResponse(success);
    return success;
}

/**
 * Handle ping command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handlePingCommand(const uint8_t* data, uint8_t length) {
    Serial.println("RS-485: Ping command received");
    
    // Send pong response
    uint8_t response[] = {0x50, 0x4F, 0x4E, 0x47}; // "PONG"
    sendDataResponse(response, 4);
    
    return true;
}

/**
 * Handle get device ID command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleGetDeviceIDCommand(const uint8_t* data, uint8_t length) {
    Serial.println("RS-485: Get device ID command received");
    
    uint8_t deviceID = getCurrentDeviceID();
    sendDataResponse(&deviceID, 1);
    
    return true;
}

/**
 * Handle set voltage command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSetVoltageCommand(const uint8_t* data, uint8_t length) {
    if (length != 2) {
        Serial.println("RS-485: Invalid voltage command length");
        return false;
    }
    
    // Extract voltage value (2 bytes, big endian)
    uint16_t voltageRaw = (data[0] << 8) | data[1];
    float voltage = voltageRaw / 100.0f; // Convert from centivolts
    
    Serial.printf("RS-485: Set voltage command: %.2fV\n", voltage);
    
    // Set voltage output
    setVoltageOutput(voltage);
    
    // Trigger status report after successful voltage setting
    printStatusReport();
    
    return true;
}

/**
 * Handle set current command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSetCurrentCommand(const uint8_t* data, uint8_t length) {
    if (length != 2) {
        Serial.println("RS-485: Invalid current command length");
        return false;
    }
    
    // Extract current value (2 bytes, big endian)
    uint16_t currentRaw = (data[0] << 8) | data[1];
    float current = currentRaw / 100.0f; // Convert from centiamperes
    
    Serial.printf("RS-485: Set current command: %.2fmA\n", current);
    
    // Set current output
    setCurrentOutput(current);
    
    // Trigger status report after successful current setting
    printStatusReport();
    
    return true;
}

/**
 * Handle set relay command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSetRelayCommand(const uint8_t* data, uint8_t length) {
    if (length != 2) {
        Serial.println("RS-485: Invalid relay command length");
        return false;
    }
    
    uint8_t relayNumber = data[0];
    uint8_t relayState = data[1];
    
    Serial.printf("RS-485: Set relay command: Relay=%d, State=%d\n", relayNumber, relayState);
    
    // Set relay state
    if (relayState == 0) {
        setRelay(relayNumber, false);
    } else {
        setRelay(relayNumber, true);
    }
    
    return true;
}

/**
 * Handle get status command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleGetStatusCommand(const uint8_t* data, uint8_t length) {
    Serial.println("RS-485: Get status command received");
    
    // Create status response
    uint8_t status[8];
    
    // Device ID
    status[0] = getCurrentDeviceID();
    
    // Current voltage and current (2 bytes each, big endian)
    float currentVoltage = getCurrentVoltage();
    float currentCurrent = getCurrentCurrent();
    
    uint16_t voltageRaw = (uint16_t)(currentVoltage * 100);
    uint16_t currentRaw = (uint16_t)(currentCurrent * 100);
    
    status[1] = (voltageRaw >> 8) & 0xFF;
    status[2] = voltageRaw & 0xFF;
    status[3] = (currentRaw >> 8) & 0xFF;
    status[4] = currentRaw & 0xFF;
    
    // Relay states (bits 0-5 for relays 1-6)
    uint8_t relayStates = 0;
    for (int i = 1; i <= 6; i++) {
        if (getRelayState(i)) {
            relayStates |= (1 << (i - 1));
        }
    }
    status[5] = relayStates;
    
    // Sine wave status
    status[6] = isSineWaveActive() ? 0x01 : 0x00;
    
    // Reserved for future use
    status[7] = 0x00;
    
    sendDataResponse(status, 8);
    
    return true;
}

/**
 * Handle sine wave command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSineWaveCommand(const uint8_t* data, uint8_t length) {
    if (length != 6) {
        Serial.println("RS-485: Invalid sine wave command length");
        return false;
    }
    
    // Extract parameters: [mode][center][amplitude][period_low][period_high][reserved]
    uint8_t mode = data[0];
    uint8_t center = data[1];
    uint8_t amplitude = data[2];
    uint16_t period = (data[3] << 8) | data[4];
    
    Serial.printf("RS-485: Sine wave command: Mode=%c, Center=%d, Amplitude=%d, Period=%dms\n", 
                  mode, center, amplitude, period);
    
    // Start sine wave generation (analog mode only)
    char modeChar;
    switch (mode) {
        case 0: modeChar = 'v'; break; // Voltage
        case 1: modeChar = 'c'; break; // Current
        default:
            Serial.println("RS-485: Invalid sine wave mode (only voltage=0, current=1 supported)");
            return false;
    }
    
    startSineWave(amplitude, period, center, 1, modeChar, false);
    
    return true;
}

/**
 * Handle stop sine wave command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleStopSineCommand(const uint8_t* data, uint8_t length) {
    Serial.println("RS-485: Stop sine wave command received");
    
    stopSineWave(0);  // Stop all channels
    
    return true;
} 