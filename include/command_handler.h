#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>

/**
 * Initialize command handler
 *
 * Initialize relays and DAC, set up I2C and pin states.
 */
void initCommandHandler();

/**
 * Process serial input commands
 *
 * Called in `loop()` to receive and process commands.
 * @param command Complete command string input by user
 */
void parseModeCommand(String params);
void parseValueCommand(String params);

#endif // COMMAND_HANDLER_H
