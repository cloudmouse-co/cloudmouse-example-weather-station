/**
 * CloudMouse SDK - Rotary Encoder Input Manager
 *
 * Hardware abstraction and event processing for rotary encoder with integrated push button.
 * Provides precise rotation tracking using ESP32 PCNT hardware and intelligent button press detection
 * with support for click, long press, and ultra-long press patterns.
 *
 * Features:
 * - Hardware-accelerated rotation counting using ESP32 PCNT (Pulse Counter) peripherals
 * - Cross-platform compatibility (ESP-IDF 4.4 vs 5.x API differences)
 * - Debounced rotation tracking with accumulated movement reporting
 * - Multi-level button press detection (click, long press, ultra-long press)
 * - Event-based architecture with consumption semantics for reliable state management
 * - Real-time press duration monitoring for dynamic UI feedback
 * - Configurable timing thresholds for different interaction patterns
 *
 * Hardware Requirements:
 * - Rotary encoder with quadrature output (CLK/DT pins)
 * - Integrated push button (SW pin) with pull-up resistor
 * - Recommended: KY-040 or similar 5V tolerant encoder module
 *
 * Pin Configuration:
 * - CLK Pin 16: Encoder Clock/A signal (quadrature phase A)
 * - DT Pin 18:  Encoder Data/B signal (quadrature phase B)
 * - SW Pin 17:  Encoder Switch/Button (active LOW with pull-up)
 *
 * Usage Pattern:
 * 1. Call init() during system initialization
 * 2. Call update() regularly in main loop (every 10-20ms recommended)
 * 3. Consume events using get*() methods (auto-reset after reading)
 * 4. Query state using is*() methods (non-consuming for UI feedback)
 *
 * Event Consumption Model:
 * - get*() methods return event state and automatically reset to false/zero
 * - Ensures each user action is processed exactly once
 * - Prevents duplicate event processing in multi-task environments
 * - Thread-safe when called from single task context
 *
 * Timing Configuration:
 * - Click: < 500ms press duration (quick selection)
 * - Long Press: >= 1000ms (context menus, settings)
 * - Ultra-Long Press: >= 3000ms (factory reset, special functions)
 *
 * Performance Characteristics:
 * - Rotation resolution: 0.25° precision (4 counts per physical detent)
 * - Update frequency: 50-100Hz recommended for responsive feel
 * - Button debouncing: Hardware pull-up + software state machine
 * - Memory usage: ~100 bytes RAM for state variables
 */

#pragma once
#include <Arduino.h>

// Hardware pin definitions
#define ENCODER_CLK_PIN 16 // Encoder Clock/A signal (quadrature phase A)
#define ENCODER_DT_PIN 18  // Encoder Data/B signal (quadrature phase B)
#define ENCODER_SW_PIN 17  // Encoder Switch/Button (active LOW)

// Platform-specific encoder library inclusion
#ifdef PLATFORMIO
#include "./RotaryEncoderPCNT.h" // Local implementation for ESP-IDF 4.4
#else
#include <RotaryEncoderPCNT.h> // Library implementation for ESP-IDF 5.x
#endif

#include "SimpleBuzzer.h"

/**
 * Rotary Encoder Manager
 *
 * Manages rotary encoder input with integrated button press detection.
 * Provides event-driven interface with automatic state reset for reliable interaction handling.
 *
 * Architecture:
 * - Hardware PCNT for precise rotation counting without CPU interrupts
 * - State machine for button press detection with timing analysis
 * - Event accumulation for smooth movement reporting across update cycles
 * - Multi-level press detection for rich interaction vocabulary
 *
 * Thread Safety:
 * - Single-task usage recommended (typically main/core task)
 * - State variables are not protected by mutexes
 * - Event consumption is atomic at method call level
 *
 * Update Frequency:
 * - Call update() every 10-20ms for responsive feel
 * - Higher frequencies improve button responsiveness
 * - Lower frequencies may miss rapid encoder movements
 */

namespace CloudMouse::Hardware
{
    class EncoderManager
    {
    public:
        /**
         * Constructor - prepares encoder manager instance
         * Initializes encoder hardware interface but does not configure GPIO
         * Call init() separately for actual hardware setup
         */
        EncoderManager();

        // ========================================================================
        // SYSTEM LIFECYCLE
        // ========================================================================

        /**
         * Initialize encoder hardware and configure GPIO pins
         * Sets up PCNT hardware, configures pin modes, and initializes state variables
         * Must be called once during system initialization before any encoder operations
         *
         * Hardware Setup:
         * - Configures CLK and DT pins for quadrature input via PCNT
         * - Enables internal pull-up on SW pin for button detection
         * - Initializes encoder position tracking and button state
         * - Sets up glitch filtering for noise immunity
         *
         * @note Not thread-safe - call only during single-threaded initialization
         */
        void init();

        /**
         * Update encoder state and process input events
         * Reads current encoder position, detects rotation movement, and processes button state
         * Must be called regularly (10-20ms intervals) for responsive input handling
         *
         * Processing Steps:
         * 1. Read current encoder position from PCNT hardware
         * 2. Calculate movement delta and accumulate for consumption
         * 3. Sample button state and detect press/release transitions
         * 4. Analyze press duration and set appropriate event flags
         * 5. Handle ongoing press feedback (buzzer, ultra-long detection)
         *
         * @note Call frequency affects responsiveness - 50-100Hz recommended
         */
        void update();

        // ========================================================================
        // EVENT CONSUMPTION INTERFACE (Auto-Reset After Reading)
        // ========================================================================

        /**
         * Get accumulated encoder movement and reset counter
         * Returns total rotation movement since last call and automatically resets to zero
         *
         * @return Rotation delta in encoder clicks
         *         Positive values = clockwise rotation
         *         Negative values = counter-clockwise rotation
         *         Zero = no movement since last call
         *
         * Resolution: 4 hardware counts per physical detent (0.25° per count)
         * Range: -32768 to +32767 (16-bit signed integer)
         *
         * Usage Examples:
         * - Menu navigation: if (getMovement() > 0) nextItem();
         * - Value adjustment: volume += getMovement() * step_size;
         * - Scroll control: scrollOffset += getMovement() * scroll_speed;
         */
        int getMovement();

        /**
         * Check for button click event and reset flag
         * Returns true if a short button press was detected and automatically resets flag
         *
         * @return true if click detected (press duration < 500ms), false otherwise
         *
         * Click Definition:
         * - Button press followed by release within 500ms
         * - Most common interaction for selection/confirmation
         * - Excludes long press and ultra-long press events
         *
         * Usage Examples:
         * - Menu selection: if (getClicked()) selectCurrentItem();
         * - Toggle functions: if (getClicked()) toggleSetting();
         * - Confirmation: if (getClicked()) confirmAction();
         */
        bool getClicked();

        /**
         * Check for long press event and reset flag
         * Returns true if a long button press was detected and automatically resets flag
         *
         * @return true if long press detected (1000ms <= duration < 3000ms), false otherwise
         *
         * Long Press Definition:
         * - Button held for 1000ms or longer but less than 3000ms
         * - Typically used for context menus or secondary functions
         * - May include haptic feedback (buzzer) at threshold
         *
         * Usage Examples:
         * - Context menus: if (getLongPressed()) showContextMenu();
         * - Settings access: if (getLongPressed()) enterSettingsMode();
         * - Mode switching: if (getLongPressed()) switchMode();
         */
        bool getLongPressed();

        /**
         * Check for ultra-long press event and reset flag
         * Returns true if an ultra-long button press was detected and automatically resets flag
         *
         * @return true if ultra-long press detected (duration >= 3000ms), false otherwise
         *
         * Ultra-Long Press Definition:
         * - Button held for 3000ms (3 seconds) or longer
         * - Reserved for critical functions like factory reset
         * - Event fires immediately when threshold is reached (not on release)
         * - Provides immediate feedback for destructive operations
         *
         * Usage Examples:
         * - Factory reset: if (getUltraLongPressed()) factoryReset();
         * - Bootloader mode: if (getUltraLongPressed()) enterBootloader();
         * - Emergency functions: if (getUltraLongPressed()) emergencyShutdown();
         */
        bool getUltraLongPressed();

        // ========================================================================
        // STATE QUERY INTERFACE (Non-Consuming, Real-Time Status)
        // ========================================================================

        /**
         * Check if button is currently pressed down
         * Real-time status query that does not affect event flags
         *
         * @return true if button is currently pressed (SW pin LOW), false if released
         *
         * Usage Examples:
         * - Visual feedback: if (isButtonDown()) highlightButton();
         * - Continuous actions: while (isButtonDown()) performAction();
         * - UI state indication: setButtonState(isButtonDown());
         */
        bool isButtonDown() const;

        /**
         * Get current button press duration in real-time
         * Returns elapsed time since button press started, zero if not pressed
         *
         * @return Current press duration in milliseconds, 0 if button not pressed
         *
         * Usage Examples:
         * - Progress indicators: updateProgressBar(getPressTime() / 3000.0f);
         * - Threshold feedback: if (getPressTime() > 1000) showLongPressHint();
         * - Dynamic UI: setButtonColor(getPressTime() > 500 ? RED : GREEN);
         */
        int getPressTime() const;

        /**
         * Get duration of last completed button press
         * Returns the duration of the most recent press-release cycle
         *
         * @return Duration of last press in milliseconds, 0 if no press recorded
         *
         * Usage Examples:
         * - Analytics: logInteraction(getLastPressDuration());
         * - Adaptive UI: adjustSensitivity(getLastPressDuration());
         * - Debug logging: SDK_LOGGER("Last press: %dms", getLastPressDuration());
         */
        int getLastPressDuration() const;

    private:
        // ========================================================================
        // HARDWARE INTERFACE
        // ========================================================================

        RotaryEncoderPCNT encoder; // PCNT-based encoder hardware interface

        // ========================================================================
        // ENCODER STATE MANAGEMENT
        // ========================================================================

        int lastValue = 0;            // Last read encoder position
        int movement = 0;             // Accumulated movement since last consumption
        bool movementPending = false; // Flag indicating movement available for consumption

        // ========================================================================
        // BUTTON STATE MANAGEMENT
        // ========================================================================

        bool lastButtonState = HIGH;      // Previous button state (HIGH = released)
        unsigned long pressStartTime = 0; // Timestamp when current press began
        int lastPressDuration = 0;        // Duration of most recent completed press

        // Event flags for consumption-based interface
        bool clickPending = false;          // Short press event ready for consumption
        bool longPressPending = false;      // Long press event ready for consumption
        bool ultraLongPressPending = false; // Ultra-long press event ready for consumption

        // State tracking for ongoing press feedback
        bool longPressBuzzed = false;        // Prevents multiple buzzer triggers during long press
        bool ultraLongPressNotified = false; // Prevents multiple ultra-long press events

        // ========================================================================
        // TIMING CONFIGURATION CONSTANTS
        // ========================================================================

        static const int ULTRA_LONG_PRESS_DURATION = 3000; // 3 seconds for ultra-long press
        static const int LONG_PRESS_DURATION = 1000;       // 1 second for long press
        static const unsigned long CLICK_TIMEOUT = 500;    // 500ms maximum for click

        // ========================================================================
        // INTERNAL PROCESSING METHODS
        // ========================================================================

        /**
         * Process encoder rotation and accumulate movement
         * Reads PCNT position, calculates delta, and updates movement accumulator
         * Called internally by update() method
         */
        void processEncoder();

        /**
         * Process button state changes and detect press events
         * Handles press/release detection, timing analysis, and event flag setting
         * Called internally by update() method
         */
        void processButton();

        /**
         * Get current press duration for ongoing press
         * Helper method for real-time press duration calculation
         *
         * @return Current press duration in milliseconds, 0 if not pressed
         */
        unsigned long getCurrentPressTime() const;
    };
};