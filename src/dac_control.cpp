#include "dac_controller.h"

// Global DAC instance definitions
GP8413 gp8413_1(0x58); // GP8413 address 0x58, corresponds to SIG1 and SIG2 voltage
GP8413 gp8413_2(0x59); // GP8413 address 0x59, corresponds to SIG3 voltage

GP8313 gp8313_1(0x5A); // GP8313 address 0x5A, corresponds to SIG1 current
GP8313 gp8313_2(0x5B); // GP8313 address 0x5B, corresponds to SIG2 current
GP8313 gp8313_3(0x5C); // GP8313 address 0x5C, corresponds to SIG3 current

// GP8413: Set voltage output
bool GP8413::setVoltage(float voltage, uint8_t channel) {
    if (voltage < 0 || voltage > 10.0) { // Ensure voltage is within 0-10V range
        Serial.printf("Voltage %.2fV out of range (0 to 10.0V).\n", voltage);
        return false;
    }

    // Convert voltage to 15-bit DAC data (15-bit resolution = 32767)
    uint16_t data = static_cast<uint16_t>((voltage / 10.0) * 32767);
    
    setDACOutVoltage(data, channel); // Call base class setting function
    return true;
}

// GP8313: Set current output
void GP8313::setDACOutElectricCurrent(uint16_t current) {
    setDACOutVoltage(current);
}

/**
 * Initialize all DAC outputs to 0
 */
void initializeDACs() {
    // Initialize GP8413
    gp8413_1.setVoltage(0.0, 0); // SIG1 voltage channel
    gp8413_1.setVoltage(0.0, 1); // SIG2 voltage channel
    gp8413_2.setVoltage(0.0, 0); // SIG3 voltage channel

    // Initialize GP8313
    gp8313_1.setDACOutElectricCurrent(0); // SIG1 current channel
    gp8313_2.setDACOutElectricCurrent(0); // SIG2 current channel
    gp8313_3.setDACOutElectricCurrent(0); // SIG3 current channel

    Serial.println("All DAC outputs initialized to 0.");
}

// Global variables to track current outputs
static float currentVoltageOutput = 0.0f;
static float currentCurrentOutput = 0.0f;

/**
 * Test I2C communication with DACs
 */
void testDACCommunication() {
    Serial.println("=== Testing DAC Communication ===");
    
    // Test GP8413_1 (Address 0x58)
    Serial.println("Testing GP8413_1 (Address 0x58)...");
    bool result1 = gp8413_1.setVoltage(1.0, 0);
    delay(100);
    gp8413_1.setVoltage(0.0, 0);
    
    // Test GP8413_2 (Address 0x59)
    Serial.println("Testing GP8413_2 (Address 0x59)...");
    bool result2 = gp8413_2.setVoltage(1.0, 0);
    delay(100);
    gp8413_2.setVoltage(0.0, 0);
    
    // Test GP8313_1 (Address 0x5A)
    Serial.println("Testing GP8313_1 (Address 0x5A)...");
    gp8313_1.setDACOutElectricCurrent(1000);
    delay(100);
    gp8313_1.setDACOutElectricCurrent(0);
    
    Serial.println("=== DAC Communication Test Complete ===");
}

/**
 * Initialize DAC controllers
 */
void initDACControllers() {
    initializeDACs();
    Serial.println("DAC controllers initialized");
    
    // Test communication
    testDACCommunication();
}

/**
 * Set voltage output
 * @param voltage Voltage value (0-10V)
 */
void setVoltageOutput(float voltage) {
    if (voltage < 0 || voltage > 10.0) {
        Serial.printf("Voltage %.2fV out of range (0-10V)\n", voltage);
        return;
    }
    
    currentVoltageOutput = voltage;
    gp8413_1.setVoltage(voltage, 0); // Set on first channel
    Serial.printf("Voltage output set to %.2fV\n", voltage);
}

/**
 * Set current output
 * @param current Current value (0-25mA)
 */
void setCurrentOutput(float current) {
    if (current < 0 || current > 25.0) {
        Serial.printf("Current %.2fmA out of range (0-25mA)\n", current);
        return;
    }
    
    currentCurrentOutput = current;
    // Convert mA to DAC data: Rset=2kΩ, 25mA = 32767 (15-bit), so 1mA = 1310.68
    gp8313_1.setDACOutElectricCurrent((uint16_t)(current * 1310.68));
    Serial.printf("Current output set to %.2fmA\n", current);
}

/**
 * Get current voltage output
 * @return Current voltage value
 */
float getCurrentVoltage() {
    return currentVoltageOutput;
}

/**
 * Get current current output
 * @return Current current value
 */
float getCurrentCurrent() {
    return currentCurrentOutput;
}
