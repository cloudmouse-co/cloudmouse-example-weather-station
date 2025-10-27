/**
 * CloudMouse SDK - Rotary Encoder Input Manager Implementation
 *
 * Implementation of hardware-accelerated rotary encoder input processing with intelligent
 * button press detection and event-driven architecture for reliable user interaction handling.
 *
 * Implementation Details:
 * - PCNT hardware integration for rotation tracking without CPU interrupts
 * - State machine approach for button press detection with precise timing
 * - Event accumulation strategy for smooth movement reporting across update cycles
 * - Multi-threshold press detection for rich interaction vocabulary
 * - Consumption-based event model preventing duplicate processing
 *
 * Performance Characteristics:
 * - Rotation tracking: Hardware PCNT with 0.25° resolution
 * - Button sampling: 50-100Hz recommended for responsive feel
 * - Memory usage: ~100 bytes RAM for state management
 * - CPU overhead: Minimal due to hardware acceleration
 *
 * Timing Analysis:
 * - Click detection: < 500ms press duration
 * - Long press: 1000-2999ms with optional buzzer feedback
 * - Ultra-long press: >= 3000ms with immediate event trigger
 * - Debouncing: Handled by hardware pull-ups and state machine logic
 */

#include "./EncoderManager.h"

namespace CloudMouse::Hardware
{
    // ============================================================================
    // INITIALIZATION AND LIFECYCLE
    // ============================================================================

    EncoderManager::EncoderManager()
        : encoder(ENCODER_CLK_PIN, ENCODER_DT_PIN)
    {
        // Constructor initializes encoder hardware interface
        // Actual GPIO configuration happens in init() method
    }

    void EncoderManager::init()
    {
        Serial.println("🎮 Initializing EncoderManager...");

        // Configure button pin with internal pull-up resistor
        // Button connects SW pin to ground when pressed (active LOW)
        pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

        // Initialize PCNT-based encoder hardware
        // Sets up quadrature decoder with glitch filtering
        encoder.init();

        // Read initial encoder position and normalize to detent resolution
        // Divide by 4 because PCNT counts 4 edges per physical detent
        lastValue = encoder.position() / 4;

        // Initialize button state by reading current pin level
        // Ensures proper state tracking from startup
        lastButtonState = digitalRead(ENCODER_SW_PIN);

        Serial.printf("✅ EncoderManager initialized successfully\n");
        Serial.printf("🎮 Pin configuration: CLK=%d, DT=%d, SW=%d\n",
                      ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);
        Serial.printf("🎮 Initial encoder position: %d\n", lastValue);
    }

    // ============================================================================
    // MAIN UPDATE LOOP
    // ============================================================================

    void EncoderManager::update()
    {
        // Process encoder rotation and button state changes
        // Call both processors to ensure comprehensive input handling
        processEncoder();
        processButton();
    }

    // ============================================================================
    // ENCODER ROTATION PROCESSING
    // ============================================================================

    void EncoderManager::processEncoder()
    {
        // Read current encoder position from PCNT hardware
        // Normalize to physical detent resolution (4 counts per detent)
        int newValue = encoder.position() / 4;

        // Check for position change since last update
        if (newValue != lastValue)
        {
            // Calculate movement delta and accumulate for consumption
            // Positive delta = clockwise, negative delta = counter-clockwise
            int delta = newValue - lastValue;
            movement += delta;
            lastValue = newValue;
            movementPending = true;

            // Optional debug logging for development and troubleshooting
            // Serial.printf("🔄 Encoder movement: %d (total pending: %d)\n", delta, movement);
        }
    }

    // ============================================================================
    // BUTTON PRESS PROCESSING
    // ============================================================================

    void EncoderManager::processButton()
    {
        // Sample current button state (LOW = pressed, HIGH = released)
        bool currentButtonState = digitalRead(ENCODER_SW_PIN);
        unsigned long currentTime = millis();

        // ========================================================================
        // PRESS DETECTION (HIGH → LOW transition)
        // ========================================================================

        if (currentButtonState != lastButtonState && currentButtonState == LOW)
        {
            // Button press detected - record timestamp and reset flags
            pressStartTime = currentTime;
            longPressBuzzed = false;        // Reset buzzer flag for new press
            ultraLongPressNotified = false; // Reset ultra-long press flag

            Serial.println("👆 Button press detected");
        }

        // ========================================================================
        // RELEASE DETECTION (LOW → HIGH transition)
        // ========================================================================

        if (currentButtonState != lastButtonState && currentButtonState == HIGH)
        {
            // Button release detected - analyze press duration and set event flags
            unsigned long pressDuration = currentTime - pressStartTime;
            lastPressDuration = pressDuration;

            Serial.printf("👆 Button released after %lu ms\n", pressDuration);

            // Classify press event based on duration thresholds
            if (pressDuration >= ULTRA_LONG_PRESS_DURATION)
            {
                // Ultra-long press: >= 3000ms
                // Note: Event may already be fired during ongoing press
                if (!ultraLongPressNotified)
                {
                    ultraLongPressPending = true;
                    ultraLongPressNotified = true;
                    Serial.println("👆🔒🔒 Ultra-long press event (on release)");
                }
            }
            else if (pressDuration >= LONG_PRESS_DURATION)
            {
                // Long press: 1000-2999ms
                longPressPending = true;
                Serial.println("👆🔒 Long press event");
            }
            else if (pressDuration < CLICK_TIMEOUT)
            {
                // Short click: < 500ms
                clickPending = true;
                Serial.println("👆 Click event");
            }
            // Note: Press durations between 500-999ms are ignored (dead zone)
        }

        // ========================================================================
        // ONGOING PRESS FEEDBACK (while button held down)
        // ========================================================================

        if (currentButtonState == LOW)
        {
            unsigned long pressTime = getCurrentPressTime();

            // Long press haptic feedback (buzzer) - triggered once at threshold
            if (pressTime >= LONG_PRESS_DURATION && !longPressBuzzed)
            {
                // Optional: Enable buzzer feedback for long press indication
                // SimpleBuzzer::buzz();
                longPressBuzzed = true;
                Serial.println("🔊 Long press threshold reached (buzz disabled)");
            }

            // Ultra-long press immediate notification (don't wait for release)
            if (pressTime >= ULTRA_LONG_PRESS_DURATION && !ultraLongPressNotified)
            {
                ultraLongPressPending = true;
                ultraLongPressNotified = true;
                Serial.println("👆🔒🔒 Ultra-long press triggered immediately!");
            }
        }
        else
        {
            // Button released - ensure ultra-long press flag is properly reset
            // This handles edge cases where timing might be inconsistent
            if (ultraLongPressNotified && lastPressDuration < ULTRA_LONG_PRESS_DURATION)
            {
                ultraLongPressNotified = false;
            }
        }

        // Update state tracking for next iteration
        lastButtonState = currentButtonState;
    }

    // ============================================================================
    // EVENT CONSUMPTION INTERFACE IMPLEMENTATION
    // ============================================================================

    int EncoderManager::getMovement()
    {
        // Return accumulated movement and reset for next consumption cycle
        if (movementPending)
        {
            int result = movement;   // Capture current movement total
            movement = 0;            // Reset accumulator for next cycle
            movementPending = false; // Clear pending flag

            Serial.printf("📊 Movement consumed: %d clicks\n", result);
            return result;
        }

        // No movement pending - return zero
        return 0;
    }

    bool EncoderManager::getClicked()
    {
        // Return click state and reset flag for next consumption cycle
        if (clickPending)
        {
            clickPending = false; // Reset flag after consumption
            Serial.println("📊 Click event consumed");
            return true;
        }

        // No click pending
        return false;
    }

    bool EncoderManager::getLongPressed()
    {
        // Return long press state and reset flag for next consumption cycle
        if (longPressPending)
        {
            longPressPending = false; // Reset flag after consumption
            Serial.println("📊 Long press event consumed");
            return true;
        }

        // No long press pending
        return false;
    }

    bool EncoderManager::getUltraLongPressed()
    {
        // Return ultra-long press state and reset flag for next consumption cycle
        if (ultraLongPressPending)
        {
            ultraLongPressPending = false; // Reset flag after consumption
            Serial.println("📊 Ultra-long press event consumed");
            return true;
        }

        // No ultra-long press pending
        return false;
    }

    // ============================================================================
    // STATE QUERY INTERFACE IMPLEMENTATION (Non-Consuming)
    // ============================================================================

    bool EncoderManager::isButtonDown() const
    {
        // Real-time button state query (LOW = pressed)
        return digitalRead(ENCODER_SW_PIN) == LOW;
    }

    int EncoderManager::getPressTime() const
    {
        // Get current press duration or zero if not pressed
        return getCurrentPressTime();
    }

    int EncoderManager::getLastPressDuration() const
    {
        // Return duration of most recent completed press cycle
        return lastPressDuration;
    }

    // ============================================================================
    // INTERNAL HELPER METHODS
    // ============================================================================

    unsigned long EncoderManager::getCurrentPressTime() const
    {
        // Calculate elapsed time since press started, or zero if not pressed
        if (digitalRead(ENCODER_SW_PIN) == LOW)
        {
            // Button currently pressed - calculate elapsed time
            return millis() - pressStartTime;
        }

        // Button not pressed - return zero
        return 0;
    }
}