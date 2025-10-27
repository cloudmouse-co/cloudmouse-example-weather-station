/**
 * CloudMouse SDK - Cross-Platform Rotary Encoder PCNT Driver
 * 
 * Hardware abstraction layer for ESP32 Pulse Counter (PCNT) peripheral with automatic
 * ESP-IDF version detection and API compatibility handling. Provides unified interface
 * for rotary encoder quadrature signal processing across different development environments.
 * 
 * Cross-Platform Compatibility:
 * - PlatformIO: ESP-IDF 4.4 API (legacy driver/pcnt.h)
 * - Arduino IDE: ESP-IDF 5.x API (new driver/pulse_cnt.h)
 * - Automatic detection and appropriate API selection at compile time
 * - Identical interface regardless of underlying ESP-IDF version
 * 
 * Hardware Features:
 * - Quadrature encoder signal processing using ESP32 PCNT hardware
 * - Hardware-based counting eliminates CPU interrupt overhead
 * - Configurable glitch filtering for electrical noise immunity  
 * - 16-bit signed counter range (-32768 to +32767)
 * - Position offset support for application-specific zero points
 * - Automatic direction detection based on phase relationship
 * 
 * Technical Specifications:
 * - Resolution: 4 counts per physical encoder detent (typical)
 * - Maximum frequency: Several MHz (limited by PCNT peripheral)
 * - Glitch filter: Configurable 1-1000ns noise suppression
 * - Counter range: ±32K counts with overflow handling
 * - Memory usage: ~50 bytes RAM per instance
 * 
 * Encoder Wiring:
 * - Pin A (CLK): Encoder quadrature signal A with pull-up resistor
 * - Pin B (DT):  Encoder quadrature signal B with pull-up resistor
 * - VCC: 3.3V or 5V depending on encoder module
 * - GND: Common ground connection
 * 
 * Quadrature Signal Processing:
 * - Channel A: Pulse counting with Channel B as direction control
 * - Channel B: Direction sensing with Channel A as reference
 * - Hardware state machine eliminates software debouncing requirements
 * - Immune to moderate electrical noise with glitch filtering
 * 
 * Usage Pattern:
 * 1. Instantiate with pin assignments: RotaryEncoderPCNT encoder(pin_a, pin_b)
 * 2. Call init() to configure PCNT hardware
 * 3. Read position() for current count value
 * 4. Use setPosition() or zero() for count reset/offset
 * 5. Call deinit() for cleanup (automatic in destructor)
 */

#ifndef ROTARY_ENCODER_PCNT_H
#define ROTARY_ENCODER_PCNT_H

#include <Arduino.h>
#include "esp_err.h"

// ============================================================================
// ESP-IDF VERSION DETECTION AND API SELECTION
// ============================================================================

#ifdef PLATFORMIO
    // PlatformIO typically uses ESP-IDF 4.4 with legacy PCNT API
    #include "driver/pcnt.h"
    #define USE_OLD_PCNT_API
#else
    // Arduino IDE uses ESP-IDF 5.x with new pulse counter API
    #include "driver/pulse_cnt.h"
    #define USE_NEW_PCNT_API
#endif

// Configuration constants
#define START_POS_DEFAULT 0         // Default starting position
#define GLITCH_NS_DEFAULT 1000      // Default glitch filter time (1µs)

/**
 * Cross-Platform Rotary Encoder PCNT Driver
 * 
 * Provides unified interface for ESP32 PCNT peripheral across different ESP-IDF versions.
 * Handles quadrature encoder signal processing with hardware acceleration and noise filtering.
 * 
 * Design Principles:
 * - Single interface for multiple ESP-IDF API versions
 * - Hardware acceleration for minimal CPU overhead
 * - Configurable filtering for noise immunity
 * - Position offset support for application flexibility
 * - Automatic resource cleanup in destructor
 * 
 * Threading Considerations:
 * - PCNT hardware is inherently thread-safe
 * - position() calls are atomic hardware reads
 * - Multiple instances can use different PCNT units
 * - No shared state between encoder instances
 */
class RotaryEncoderPCNT {
public:
    // ========================================================================
    // CONSTRUCTORS - Flexible initialization options
    // ========================================================================
    
    /**
     * Full parameter constructor
     * Creates encoder instance with complete configuration
     * 
     * @param a GPIO pin number for encoder signal A (CLK)
     * @param b GPIO pin number for encoder signal B (DT)
     * @param start_pos Initial position offset value
     * @param glitch_ns Glitch filter time in nanoseconds (noise immunity)
     */
    RotaryEncoderPCNT(int a, int b, int start_pos, uint16_t glitch_ns) {
        glitch_time = glitch_ns;
        offset = start_pos;
        pin_a = a;
        pin_b = b;
    }

    /**
     * Constructor with starting position
     * Uses default glitch filter setting
     * 
     * @param a GPIO pin number for encoder signal A (CLK)
     * @param b GPIO pin number for encoder signal B (DT)  
     * @param start_pos Initial position offset value
     */
    RotaryEncoderPCNT(int a, int b, int start_pos) {
        offset = start_pos;
        pin_a = a;
        pin_b = b;
        glitch_time = GLITCH_NS_DEFAULT;
    }

    /**
     * Basic constructor with pin assignment
     * Uses default starting position and glitch filter
     * 
     * @param a GPIO pin number for encoder signal A (CLK)
     * @param b GPIO pin number for encoder signal B (DT)
     */
    RotaryEncoderPCNT(int a, int b) {
        pin_a = a;
        pin_b = b;
        offset = START_POS_DEFAULT;
        glitch_time = GLITCH_NS_DEFAULT;
    }

    /**
     * Default constructor
     * Requires manual pin assignment before init()
     */
    RotaryEncoderPCNT() {
        pin_a = 255;
        pin_b = 255;
        offset = START_POS_DEFAULT;
        glitch_time = GLITCH_NS_DEFAULT;
    }

    /**
     * Destructor with automatic cleanup
     * Ensures proper PCNT hardware deinitialization
     */
    ~RotaryEncoderPCNT() {
        deinit();
    }
    
    // ========================================================================
    // HARDWARE INITIALIZATION AND CONTROL
    // ========================================================================
    
    /**
     * Initialize PCNT hardware for quadrature encoder processing
     * Configures GPIO pins, PCNT channels, and glitch filtering
     * 
     * Configuration Steps:
     * 1. Set GPIO pins as inputs with pull-up resistors
     * 2. Configure PCNT channels for quadrature decoding
     * 3. Set up glitch filter for noise immunity
     * 4. Initialize counter and start operation
     * 
     * Platform Handling:
     * - ESP-IDF 4.4: Uses legacy pcnt_config_t structure
     * - ESP-IDF 5.x: Uses new pcnt_unit_config_t structure
     * - Automatic API selection based on compile-time detection
     * 
     * @note Must be called before position reading operations
     * @note Not thread-safe during initialization - call from single thread
     */
    void init();
    
    /**
     * Deinitialize PCNT hardware and free resources
     * Stops counter operation and releases PCNT peripheral
     * 
     * Cleanup Operations:
     * - Stop PCNT counter operation
     * - Disable PCNT unit and channels  
     * - Free allocated handles (ESP-IDF 5.x)
     * - Reset internal state variables
     * 
     * @note Safe to call multiple times
     * @note Automatic cleanup in destructor
     */
    void deinit();
    
    // ========================================================================
    // POSITION READING AND CONTROL
    // ========================================================================
    
    /**
     * Read current encoder position with offset applied
     * Returns hardware counter value plus configured offset
     * 
     * @return Current position as signed integer
     *         Positive values = clockwise from zero
     *         Negative values = counter-clockwise from zero
     *         Range: -32768 to +32767 (with offset)
     * 
     * Technical Notes:
     * - Hardware atomic read operation
     * - Includes configured position offset
     * - Error handling returns last valid value (ESP-IDF 4.4)
     * - Thread-safe for concurrent access
     */
    int position();
    
    /**
     * Set encoder position with counter reset
     * Updates offset and clears hardware counter to establish new zero point
     * 
     * @param pos New position value to set as current
     * 
     * Operations:
     * 1. Update internal offset to target position
     * 2. Clear hardware counter to zero
     * 3. Subsequent reads return: hardware_count + new_offset
     * 
     * Use Cases:
     * - Establish application-specific zero points
     * - Reset position after reaching limits
     * - Synchronize with external position references
     */
    void setPosition(int pos);
    
    /**
     * Reset encoder to default starting position
     * Convenience method equivalent to setPosition(START_POS_DEFAULT)
     * 
     * Resets both hardware counter and offset to initial state
     */
    void zero();
    
    // ========================================================================
    // PUBLIC CONFIGURATION MEMBERS
    // ========================================================================
    
    uint8_t pin_a = 255;                    // GPIO pin for encoder signal A (CLK)
    uint8_t pin_b = 255;                    // GPIO pin for encoder signal B (DT)  
    uint16_t glitch_time = GLITCH_NS_DEFAULT; // Glitch filter time in nanoseconds

private:
    // ========================================================================
    // ESP-IDF 5.x SPECIFIC HANDLES (New API)
    // ========================================================================
    
#ifdef USE_NEW_PCNT_API
    pcnt_unit_handle_t unit = nullptr;      // PCNT unit handle
    pcnt_channel_handle_t chan_a = nullptr; // Channel A handle
    pcnt_channel_handle_t chan_b = nullptr; // Channel B handle
#endif
    
    // ========================================================================
    // INTERNAL STATE VARIABLES
    // ========================================================================
    
    int16_t low_limit = INT16_MIN;          // Counter lower limit
    int16_t high_limit = INT16_MAX;         // Counter upper limit
    int count = 0;                          // Internal count variable (unused in current implementation)
    int offset = START_POS_DEFAULT;         // Position offset for zero point adjustment
};

#endif