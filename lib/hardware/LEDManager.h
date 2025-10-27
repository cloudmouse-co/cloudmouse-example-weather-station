/**
 * CloudMouse SDK - LED Animation Manager
 *
 * Manages the 12-LED NeoPixel ring with hardware abstraction and animation effects.
 * Provides thread-safe event-driven interface for controlling LED states and animations.
 *
 * Features:
 * - Dedicated FreeRTOS task for smooth 50Hz animation rendering
 * - Priority-based animation state machine
 * - Thread-safe communication via event queues
 * - Multiple animation modes: init, loading, pulsating, flash effects
 * - Configurable color themes with preferences storage
 */

#pragma once
#include <Adafruit_NeoPixel.h>
#include "../prefs/PreferencesManager.h"
#include "../core/Events.h"

#define NUM_LEDS 12
#define DATA_PIN 15

namespace CloudMouse::Hardware
{
    /**
     * LED Event Types for inter-task communication
     */
    enum class LEDEventType
    {
        SET_LOADING,   // Enable/disable loading animation
        FLASH_COLOR,   // Trigger color flash with duration
        ACTIVATE,      // Activate LEDs on user interaction
        SET_COLOR,     // Change base color theme
        SET_BRIGHTNESS // Adjust global brightness
    };

    /**
     * LED Event Structure
     * Used for thread-safe communication between tasks
     */
    struct LEDEvent
    {
        LEDEventType type;
        uint8_t r, g, b; // RGB color values (0-255)
        int brightness;  // Brightness level (0-255)
        int duration;    // Duration in milliseconds
        bool state;      // Boolean state for on/off operations
    };

    /**
     * LED Animation Manager
     *
     * Controls the NeoPixel LED ring with various animation effects.
     * Runs on dedicated FreeRTOS task for consistent timing.
     *
     * Animation Priority Order:
     * 1. Fade transitions
     * 2. Flash effects
     * 3. Loading animation
     * 4. Init boot sequence
     * 5. Idle pulsating
     */
    class LEDManager
    {
    public:
        LEDManager();
        ~LEDManager() = default;

        // System lifecycle
        void init();               // Initialize hardware and task
        void startAnimationTask(); // Start animation FreeRTOS task
        void stopAnimationTask();  // Stop and cleanup animation task

        // Public interface (thread-safe)
        void setLoadingState(bool on); // Control loading animation state
        void flashColor(uint8_t r, uint8_t g, uint8_t b, int brightness, int duration);
        void activate();                                 // Trigger activation animation
        void updateLastEncoderMovementTime();            // Update activity timestamp
        void setMainColor(const String &colorName = ""); // Set color theme

        // State queries
        bool isLoading() const { return loading; }
        bool isPulsating() const { return pulsating; }
        int getBrightness() const { return currentBrightness; }
        bool isInitAnimationComplete() const { return initAnimationCompleted; }

        // Task management
        TaskHandle_t getAnimationTaskHandle() const { return animationTaskHandle; }
        void restartAnimationTask(); // Restart task if needed

    private:
        // Hardware
        Adafruit_NeoPixel strip;

        // Task management
        TaskHandle_t animationTaskHandle = nullptr;
        QueueHandle_t ledEventQueue = nullptr;
        static const int LED_QUEUE_SIZE = 10;

        // Animation state flags
        bool pulsating = true;               // Idle breathing effect
        bool loading = false;                // Loading animation active
        bool flash = false;                  // Flash effect active
        bool fading = false;                 // Brightness fade active
        bool inited = false;                 // Boot sequence completed
        bool initAnimationCompleted = false; // Full init sequence done

        // Timing variables
        unsigned long previousMillis = 0;
        unsigned long lastEncMovementTime = 0;
        unsigned long lastFlashStarted = 0;
        unsigned long fadeStartMillis = 0;

        // Animation parameters
        int cursorLED = 0;           // Current LED for sweep animations
        int currentBrightness = 250; // Current brightness level
        int startBrightness = 0;     // Fade start brightness
        int targetBrightness = 0;    // Fade target brightness
        int fadeDuration = 3000;     // Fade duration in milliseconds
        int flashDuration = 0;       // Flash duration in milliseconds
        bool clockwise = true;       // Animation direction

        // Color state
        uint8_t baseRed = 0, baseGreen = 181, baseBlue = 214; // Base color (azure)
        uint8_t red = 0, green = 0, blue = 0;                 // Current RGB values

        // Configuration constants
        static const unsigned long ANIMATION_INTERVAL = 70; // Init sweep timing
        static const int IDLE_DELAY_SECONDS = 5;            // Idle timeout

        // FreeRTOS task functions
        static void animationTaskFunction(void *parameter); // Task entry point

        // Animation system
        void animationLoop();    // Main animation loop (50Hz)
        void processLEDEvents(); // Process incoming events
        void updateAnimations(); // Update active animations

        // Animation handlers
        void updateInitAnimation();      // Boot sequence LED sweep
        void updateLoadingAnimation();   // Loading pulse effect
        void updatePulsatingAnimation(); // Idle breathing animation
        void updateFlashAnimation();     // Flash effect timing
        void updateFadeAnimation();      // Brightness fade transitions

        // Helper functions
        void resetAllLEDs();                                 // Turn off all LEDs
        void setAllLEDs(uint8_t r, uint8_t g, uint8_t b);    // Set all LEDs to color
        void fadeToBrightness(int brightness, int duration); // Start brightness fade

        // Communication
        bool sendLEDEvent(const LEDEvent &event); // Send event to animation task
    };

} // namespace CloudMouse