#ifndef RS485_COMMAND_HANDLER_H
#define RS485_COMMAND_HANDLER_H

#include "rs485_serial.h"

// Command types
#define CMD_PING 0x01
#define CMD_GET_DEVICE_ID 0x02
#define CMD_SET_VOLTAGE 0x10
#define CMD_SET_CURRENT 0x11
#define CMD_SET_RELAY 0x20
#define CMD_GET_STATUS 0x30
#define CMD_SINE_WAVE 0x40
#define CMD_STOP_SINE 0x41

// Response codes
#define RESP_SUCCESS 0x01
#define RESP_ERROR 0x00
#define RESP_INVALID_COMMAND 0x02
#define RESP_INVALID_PARAMETER 0x03

/**
 * Initialize RS-485 command handler
 */
void initRS485CommandHandler();

/**
 * Process RS-485 commands and execute corresponding actions
 * @return true if a command was processed
 */
bool handleRS485Commands();

/**
 * Execute a specific command
 * @param command Pointer to the command structure
 * @return true if command was executed successfully
 */
bool executeRS485Command(RS485Command* command);

/**
 * Handle ping command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handlePingCommand(const uint8_t* data, uint8_t length);

/**
 * Handle get device ID command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleGetDeviceIDCommand(const uint8_t* data, uint8_t length);

/**
 * Handle set voltage command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSetVoltageCommand(const uint8_t* data, uint8_t length);

/**
 * Handle set current command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSetCurrentCommand(const uint8_t* data, uint8_t length);

/**
 * Handle set relay command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSetRelayCommand(const uint8_t* data, uint8_t length);

/**
 * Handle get status command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleGetStatusCommand(const uint8_t* data, uint8_t length);

/**
 * Handle sine wave command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleSineWaveCommand(const uint8_t* data, uint8_t length);

/**
 * Handle stop sine wave command
 * @param data Command data
 * @param length Data length
 * @return true if successful
 */
bool handleStopSineCommand(const uint8_t* data, uint8_t length);

#endif // RS485_COMMAND_HANDLER_H 