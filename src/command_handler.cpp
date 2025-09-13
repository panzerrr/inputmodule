#include "command_handler.h"
#include "dac_controller.h"
#include "relay_controller.h"
#include "utils.h"

// Global variable declarations
extern char signalModes[3]; // Signal modes

// Global signal mapping table
SignalMap signalMap[3] = {
    {&gp8413_1, 0, &gp8313_1}, // SIG1
    {&gp8413_1, 1, &gp8313_2}, // SIG2
    {&gp8413_2, 0, &gp8313_3}  // SIG3
};

void parseModeCommand(String params) {
    int commaIndex = params.indexOf(',');
    if (commaIndex == -1 || params.length() <= commaIndex + 1) {
        Serial.println("Invalid mode command. Use 'MODE SIG,MODE' (case-insensitive).");
        return;
    }

    int sig = params.substring(0, commaIndex).toInt(); // Get signal number
    char mode = toLowerCase(params.substring(commaIndex + 1).charAt(0)); // Get mode ('v' or 'c') - case insensitive

    if (sig < 1 || sig > 3 || (mode != 'v' && mode != 'c')) {
        Serial.println("Invalid mode. Use 'v' or 'c' (case-insensitive).");
        return;
    }

    // Execute protection operation
    if (mode == 'v') {
        signalMap[sig - 1].currentDAC->setDACOutElectricCurrent(0);
        Serial.printf("SIG%d: Current set to 0mA for protection.\n", sig);
    } else if (mode == 'c') {
        signalMap[sig - 1].voltageDAC->setVoltage(0.0, signalMap[sig - 1].voltageChannel);
        Serial.printf("SIG%d: Voltage set to 0V for protection.\n", sig);
    }

    // Update mode status and set relay
    signalModes[sig - 1] = mode;
    setRelayMode(sig, mode);
    Serial.printf("Mode set: SIG%d -> %c\n", sig, mode);
}

void parseValueCommand(String params) {
    int commaIndex = params.indexOf(',');
    if (commaIndex == -1 || params.length() <= commaIndex + 1) {
        Serial.println("Invalid value command. Use 'VALUE SIG,VALUE' (case-insensitive).");
        return;
    }

    int sig = params.substring(0, commaIndex).toInt();
    float value = params.substring(commaIndex + 1).toFloat();

    if (sig < 1 || sig > 3) {
        Serial.println("Invalid signal number. Use 1 to 3.");
        return;
    }

    char mode = signalModes[sig - 1];
    if (mode == 'v') {
        if (value < 0 || value > 10.0) {
            Serial.println("Invalid voltage value. Use 0-10V.");
            return;
        }
        signalMap[sig - 1].voltageDAC->setVoltage(value, signalMap[sig - 1].voltageChannel);
        Serial.printf("Voltage set: SIG%d -> %.2f V\n", sig, value);
    } else if (mode == 'c') {
        if (value < 0 || value > 25.0) {
            Serial.println("Invalid current value. Use 0-25mA.");
            return;
        }
        // Convert mA to DAC data: Rset=2kΩ, 25mA = 32767 (15-bit), so 1mA = 1310.68
        signalMap[sig - 1].currentDAC->setDACOutElectricCurrent(static_cast<uint16_t>(value * 1310.68));
        Serial.printf("Current set: SIG%d -> %.2f mA\n", sig, value);
    } else {
        Serial.printf("Unknown mode '%c' for SIG%d.\n", mode, sig);
    }
}
