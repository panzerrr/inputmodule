#include "device_id.h"

/**
 * Example: How to use Device ID functionality in main program
 * 
 * This file demonstrates how to integrate device ID detection
 * into your main application for device identification.
 */

void setupDeviceID() {
    // Initialize device ID pins
    initDeviceIDPins();
    
    // Calculate device ID from hardware jumpers
    uint8_t deviceID = calculateDeviceID();
    
    // Display device information
    Serial.println("=== DEVICE ID DETECTION ===");
    Serial.printf("Device ID: %d (0x%02X)\n", deviceID, deviceID);
    
    // Show jumper status
    Serial.println("Jumper Status:");
    Serial.printf("  NO1 (IO23): %s\n", digitalRead(NO1) == LOW ? "Grounded (Active=1)" : "Floating (Inactive=0)");
    Serial.printf("  NO2 (IO12): %s\n", digitalRead(NO2) == LOW ? "Grounded (Active=1)" : "Floating (Inactive=0)");
    Serial.printf("  NO3 (IO4):  %s\n", digitalRead(NO3) == LOW ? "Grounded (Active=1)" : "Floating (Inactive=0)");
    Serial.printf("  NO4 (IO5):  %s\n", digitalRead(NO4) == LOW ? "Grounded (Active=1)" : "Floating (Inactive=0)");
    Serial.printf("  NO5 (IO32): %s\n", digitalRead(NO5) == LOW ? "Grounded (Active=1)" : "Floating (Inactive=0)");
    
    // Binary representation
    Serial.printf("Binary: %c%c%c%c%c\n", 
                  (deviceID & 0x10) ? '1' : '0',  // NO5
                  (deviceID & 0x08) ? '1' : '0',  // NO4
                  (deviceID & 0x04) ? '1' : '0',  // NO3
                  (deviceID & 0x02) ? '1' : '0',  // NO2
                  (deviceID & 0x01) ? '1' : '0'); // NO1
    
    Serial.println("==========================");
    
    // Use device ID for configuration
    switch(deviceID) {
        case 0:
            Serial.println("Device 0: Default configuration");
            break;
        case 1:
            Serial.println("Device 1: Special configuration A");
            break;
        case 2:
            Serial.println("Device 2: Special configuration B");
            break;
        case 3:
            Serial.println("Device 3: Special configuration C");
            break;
        default:
            Serial.printf("Device %d: Custom configuration\n", deviceID);
            break;
    }
}

/**
 * Example: Use device ID for Modbus slave address
 */
uint8_t getModbusSlaveAddress() {
    uint8_t deviceID = calculateDeviceID();
    // Use device ID as base address, add offset if needed
    return 0x01 + deviceID; // Addresses: 0x01, 0x02, 0x03, etc.
}

/**
 * Example: Check if device has specific feature enabled
 */
bool isFeatureEnabled(uint8_t featureBit) {
    uint8_t deviceID = calculateDeviceID();
    return (deviceID & (1 << featureBit)) != 0;
}

/**
 * Example: Get device configuration based on ID
 */
struct DeviceConfig {
    uint8_t modbusAddress;
    uint32_t baudRate;
    bool enableAdvancedFeatures;
    char deviceName[16];
};

DeviceConfig getDeviceConfig() {
    uint8_t deviceID = calculateDeviceID();
    DeviceConfig config;
    
    config.modbusAddress = 0x01 + deviceID;
    config.baudRate = 19200;
    config.enableAdvancedFeatures = (deviceID > 0);
    sprintf(config.deviceName, "INPUT_MOD_%02d", deviceID);
    
    return config;
} 