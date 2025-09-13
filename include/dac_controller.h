#ifndef DAC_CONTROLLER_H
#define DAC_CONTROLLER_H

#include "DFRobot_GP8XXX.h"
#include <Arduino.h>

// GP8413 class definition: for voltage output
class GP8413 : public DFRobot_GP8XXX_IIC {
public:
    GP8413(uint8_t deviceAddr = DFGP8XXX_I2C_DEVICEADDR, uint16_t resolution = RESOLUTION_15_BIT)
        : DFRobot_GP8XXX_IIC(resolution, deviceAddr) {}

    /**
     * Set voltage output
     * @param voltage Target output voltage (unit: V), range 0-10V
     * @param channel Output channel (0 or 1)
     * @return Returns true on success, false on failure
     */
    bool setVoltage(float voltage, uint8_t channel = 0);
};

// GP8313 class definition: for current output
class GP8313 : public DFRobot_GP8XXX_IIC {
public:
    GP8313(uint8_t deviceAddr, uint16_t resolution = RESOLUTION_15_BIT)
        : DFRobot_GP8XXX_IIC(resolution, deviceAddr) {}

    /**
     * Set current output
     * @param current Target output current (unit: μA), range 0-25000μA
     */
    void setDACOutElectricCurrent(uint16_t current);
};

// Global DAC instance declarations
extern GP8413 gp8413_1;
extern GP8413 gp8413_2;

extern GP8313 gp8313_1;
extern GP8313 gp8313_2;
extern GP8313 gp8313_3;

/**
 * Initialize all DACs
 * Set all outputs to 0
 */
void initializeDACs();

/**
 * Initialize DAC controllers
 */
void initDACControllers();

/**
 * Set voltage output
 * @param voltage Voltage value (0-10V)
 */
void setVoltageOutput(float voltage);

/**
 * Set current output
 * @param current Current value (0-25mA)
 */
void setCurrentOutput(float current);

/**
 * Get current voltage output
 * @return Current voltage value
 */
float getCurrentVoltage();

/**
 * Get current current output
 * @return Current current value
 */
float getCurrentCurrent();

#endif // DAC_CONTROLLER_H
