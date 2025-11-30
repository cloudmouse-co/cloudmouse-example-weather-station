#pragma once

#include <Arduino.h> // Necessario per Serial.printf
#include "../config/DeviceConfig.h"


// ========================================================================
// APP_LOGGER Macro definition
// ========================================================================

#if defined(APP_DEBUGGER_ACTIVE) && APP_DEBUGGER_ACTIVE

    /**
     * @brief It prints a message with [APP] prefix.
     * * This macro uses Serial.printf to let params formatting.
     * a new line character is added ('\n') at the end of the string.
     * 
     * - Example: APP_LOGGER("Value: %d", myValue);
     */
    #define APP_LOGGER(format, ...) \
        Serial.printf("[APP] " format "\n", ##__VA_ARGS__)

#else

    /**
     * @brief disabled macro version.
     * * When APP_DEBUGGER_ACTIVE is false, this macro resolves in a 
     * 'do/while(0)' empty block, which assures no interactions with the code 
     * with no added overhead in runtime.
     */
    #define APP_LOGGER(format, ...) do { } while (0)

#endif


// ========================================================================
// SDK_LOGGER Macro definition
// ========================================================================

#if defined(SDK_DEBUGGER_ACTIVE) && SDK_DEBUGGER_ACTIVE

    /**
     * @brief It prints a message with [SDK] prefix.
     * * This macro uses Serial.printf to let params formatting.
     * a new line character is added ('\n') at the end of the string.
     * 
     * - Example: APP_LOGGER("Value: %d", myValue);
     */
    #define SDK_LOGGER(format, ...) \
        Serial.printf("[SDK] " format "\n", ##__VA_ARGS__)

#else

    /**
     * @brief disabled macro version.
     * * When SDK_DEBUGGER_ACTIVE is false, this macro resolves in a 
     * 'do/while(0)' empty block, which assures no interactions with the code 
     * with no added overhead in runtime.
     */
    #define SDK_LOGGER(format, ...) do { } while (0)

#endif