/*
EXPERIMENTAL INTEGRATION EXAMPLE
This file shows how to integrate the sine wave generator into the main code.
DO NOT include this file in your main build - it's for reference only.
*/

// To enable sine wave generator, uncomment these lines in main.cpp:

/*
// Add to includes section:
#include "sine_wave_generator.h"

// Add to setup() function after other initializations:
initSineWaveGenerator();

// Add to loop() function in the command parsing section:
} else if (upperInput.startsWith("SINE")) {
    // Experimental sine wave generator
    parseSineWaveCommand(input);
} else {
    // ... existing error handling
}

// Add to loop() function at the end (before delay):
updateSineWave(); // Update sine wave output
*/

/*
EXAMPLE USER SESSION WITH SINE WAVE GENERATOR:

=== MODE SELECTION ===
Please select your operating mode:
D - Digital Mode (Modbus Simulation)
A - Analogue Mode (Direct Output Control)
Enter 'D' or 'A':

User: A
System: Analogue Mode (Direct Output Control) selected.
System: Commands (case-insensitive):
System: - MODE SIG,MODE  (e.g., mode 1,V or MODE 2,C)
System: - VALUE SIG,VALUE  (e.g., value 1,5.0 or VALUE 2,10.0)
System: - SWITCH - Switch to Digital Mode
System: - SINE START amplitude period signal mode (experimental)
System: - SINE STOP (experimental)
System: - SINE STATUS (experimental)
System: System Initialized. Ready for commands.

User: SINE START 5.0 2.0 1 V
System: Sine wave started: Signal 1, voltage mode, 5.00V amplitude, 2.0s period

User: SINE STATUS
System: === SINE WAVE STATUS ===
System: Status: ACTIVE
System: Amplitude: 5.00
System: Period: 2.0 seconds
System: Elapsed time: 1.5 seconds
System: Progress: 75.0%
System: Center point: 5.00
System: ========================

User: SINE STOP
System: Sine wave stopped.
System: All outputs reset to 0.
*/ 