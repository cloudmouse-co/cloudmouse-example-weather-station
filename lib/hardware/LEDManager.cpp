/**
 * CloudMouse SDK - LED Animation Manager Implementation
 *
 * Implementation of the LED animation system with thread-safe event handling
 * and smooth animation effects for the NeoPixel LED ring.
 */

#include "./LEDManager.h"
#include <string>
#include "../utils/Logger.h"

namespace CloudMouse::Hardware
{

    using CloudMouse::Prefs::PreferencesManager;

    // ============================================================================
    // SYSTEM INITIALIZATION
    // ============================================================================

    LEDManager::LEDManager() : strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800)
    {
        // Initialize NeoPixel strip configuration
    }

    void LEDManager::init()
    {
        SDK_LOGGER("💡 Initializing LEDManager...");

        // Initialize NeoPixel hardware
        strip.begin();
        strip.setBrightness(currentBrightness);
        resetAllLEDs();
        strip.show();

        // Create event queue for thread-safe communication
        ledEventQueue = xQueueCreate(LED_QUEUE_SIZE, sizeof(LEDEvent));
        if (!ledEventQueue)
        {
            SDK_LOGGER("❌ Failed to create LED event queue!");
            return;
        }

        // Load user's preferred color theme
        setMainColor();

        SDK_LOGGER("✅ LEDManager initialized successfully");
    }

    void LEDManager::startAnimationTask()
    {
        if (animationTaskHandle)
        {
            SDK_LOGGER("💡 LED Animation task already running");
            return;
        }

        // Create animation task with priority 2 on Core 1
        xTaskCreatePinnedToCore(
            animationTaskFunction,
            "LED_Animation",
            8192, // Stack size
            this, // Pass instance
            2,    // Priority 2 (below UI task)
            &animationTaskHandle,
            1 // Core 1
        );

        if (animationTaskHandle)
        {
            SDK_LOGGER("✅ LED Animation task started on Core 1");
        }
        else
        {
            SDK_LOGGER("❌ Failed to start LED Animation task!");
        }
    }

    void LEDManager::stopAnimationTask()
    {
        if (animationTaskHandle)
        {
            vTaskDelete(animationTaskHandle);
            animationTaskHandle = nullptr;
            SDK_LOGGER("💡 LED Animation task stopped");
        }
    }

    // ============================================================================
    // FREERTOS TASK FUNCTIONS
    // ============================================================================

    void LEDManager::animationTaskFunction(void *parameter)
    {
        LEDManager *ledManager = static_cast<LEDManager *>(parameter);
        ledManager->animationLoop();
    }

    void LEDManager::animationLoop()
    {
        TickType_t lastWake = xTaskGetTickCount();
        uint32_t loopCounter = 0;

        SDK_LOGGER("💡 LED Animation loop started");

        while (true)
        {
            loopCounter++;

            // Health monitoring every 1000 loops
            if (loopCounter % 1000 == 0)
            {
                SDK_LOGGER("💡 LED Task alive - loops: %d, free stack: %d\n",
                              loopCounter, uxTaskGetStackHighWaterMark(NULL));
            }

            // Process incoming events from other tasks
            processLEDEvents();

            // Update animation state machine
            updateAnimations();

            // 50Hz update rate (20ms intervals)
            vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(20));
        }
    }

    // ============================================================================
    // EVENT PROCESSING SYSTEM
    // ============================================================================

    void LEDManager::processLEDEvents()
    {
        LEDEvent event;

        // Process all pending events (non-blocking)
        while (xQueueReceive(ledEventQueue, &event, 0) == pdPASS)
        {
            switch (event.type)
            {
            case LEDEventType::SET_LOADING:
                loading = event.state;
                if (loading)
                {
                    lastEncMovementTime = millis();
                    pulsating = false;
                    fading = false;
                    setAllLEDs(244, 70, 17); // Orange loading color
                }
                else
                {
                    setAllLEDs(baseRed, baseGreen, baseBlue);
                    fadeToBrightness(200, 150);
                }
                break;

            case LEDEventType::FLASH_COLOR:
                // Override all other animations for immediate feedback
                pulsating = false;
                fading = false;
                flash = true;
                flashDuration = event.duration;
                lastFlashStarted = millis();
                currentBrightness = event.brightness;

                strip.setBrightness(event.brightness);
                setAllLEDs(event.r, event.g, event.b);
                strip.show();
                break;

            case LEDEventType::ACTIVATE:
                if (!loading)
                {
                    lastEncMovementTime = millis();
                }
                pulsating = false;
                fading = false;
                currentBrightness = 255;

                strip.setBrightness(255);
                setAllLEDs(red, green, blue);
                strip.show();
                break;

            case LEDEventType::SET_COLOR:
                // Update color theme
                setAllLEDs(event.r, event.g, event.b);
                baseRed = event.r;
                baseGreen = event.g;
                baseBlue = event.b;
                red = event.r;
                green = event.g;
                blue = event.b;
                strip.show();
                break;

            case LEDEventType::SET_RAINBOW:
                rainbow = event.state;
                if (rainbow)
                {
                    // Quando l'arcobaleno inizia, ferma tutto il resto
                    pulsating = false;
                    fading = false;
                    loading = false;
                    flash = false;
                }
                else
                {
                    // Quando l'arcobaleno finisce, torna al colore base
                    setAllLEDs(baseRed, baseGreen, baseBlue);
                    strip.show();
                    // Potresti voler far tornare in stato di 'pulsating'
                    pulsating = true;
                }
                break;

            default:
                // Unknown event type
                break;
            }
        }
    }

    // ============================================================================
    // ANIMATION STATE MACHINE
    // ============================================================================

    void LEDManager::updateAnimations()
    {
        unsigned long currentMillis = millis();

        // Animation priority order:
        // 1. Fade (smooth transitions)
        // 2. Flash (immediate user feedback)
        // 3. Loading (WiFi connection state)
        // 4. Rainbow (Rainbow animation effect)
        // 5. Init (boot sequence)
        // 6. Pulsating (idle breathing)

        if (fading)
        {
            updateFadeAnimation();
            return;
        }

        if (flash)
        {
            updateFlashAnimation();
            return;
        }

        if (loading)
        {
            updateLoadingAnimation();
            return;
        }

        if (rainbow)
        {
            updateRainbowAnimation();
            return;
        }

        if (!initAnimationCompleted)
        {
            updateInitAnimation();
            return;
        }

        if (pulsating)
        {
            updatePulsatingAnimation();
            return;
        }

        // Auto-return to pulsating after idle time
        if (currentMillis - lastEncMovementTime >= IDLE_DELAY_SECONDS * 1000)
        {
            if (currentBrightness > 10)
            {
                fadeToBrightness(10, 1000);
            }
            else
            {
                pulsating = true;
            }
        }
    }

    // ============================================================================
    // ANIMATION IMPLEMENTATIONS
    // ============================================================================

    void LEDManager::updateInitAnimation()
    {
        unsigned long currentMillis = millis();

        // Boot sequence: LED sweep animation
        if (!inited && currentMillis - previousMillis >= ANIMATION_INTERVAL)
        {
            previousMillis = currentMillis;

            resetAllLEDs();
            strip.setPixelColor(cursorLED, strip.Color(red, green, blue));
            strip.show();

            // Move cursor with bounce at ends
            if (clockwise)
            {
                cursorLED++;
            }
            else
            {
                cursorLED--;
            }

            if (cursorLED >= NUM_LEDS)
            {
                clockwise = false;
                cursorLED = NUM_LEDS - 1;
            }

            if (cursorLED <= 0 && !clockwise)
            {
                // Sweep complete - fade out
                strip.setBrightness(0);
                strip.show();
                vTaskDelay(pdMS_TO_TICKS(500));

                fadeToBrightness(255, 150);
                inited = true;
                cursorLED = 0;
                clockwise = true;
            }
        }

        // Final fade to complete init
        if (inited && currentBrightness != 0)
        {
            fadeToBrightness(0, 3000);
        }

        // Init animation timeout (4 second max)
        if (currentMillis > 4000 && !initAnimationCompleted)
        {
            initAnimationCompleted = true;
            pulsating = true;
            SDK_LOGGER("💡 LED Init animation completed!");
        }
    }

    void LEDManager::updateLoadingAnimation()
    {
        static unsigned long lastLoadingUpdate = 0;
        static bool loadingUp = true;

        // Simple loading animation - alternates brightness
        if (getBrightness() != 250)
        {
            fadeToBrightness(250, 50);
        }

        if (getBrightness() != 1)
        {
            fadeToBrightness(1, 50);
        }
    }

    void LEDManager::updatePulsatingAnimation()
    {
        static unsigned long lastPulseUpdate = 0;
        static bool pulseUp = true;

        // Gentle breathing animation for idle state
        if (millis() - lastPulseUpdate > 100)
        { // 10Hz pulsating
            lastPulseUpdate = millis();

            if (pulseUp)
            {
                if (currentBrightness < 100)
                {
                    fadeToBrightness(100, 1500);
                }
                else
                {
                    pulseUp = false;
                }
            }
            else
            {
                if (currentBrightness > 10)
                {
                    fadeToBrightness(10, 2000);
                }
                else
                {
                    pulseUp = true;
                }
            }
        }
    }

    void LEDManager::updateFlashAnimation()
    {
        // Simple flash timer
        if (millis() - lastFlashStarted >= flashDuration)
        {
            flash = false;
            flashDuration = 0;
            setAllLEDs(baseRed, baseGreen, baseBlue);
            strip.show();
        }
    }

    void LEDManager::updateFadeAnimation()
    {
        unsigned long elapsedTime = millis() - fadeStartMillis;

        if (elapsedTime <= fadeDuration)
        {
            // Smooth brightness interpolation
            int brightness = map(elapsedTime, 0, fadeDuration, startBrightness, targetBrightness);
            strip.setBrightness(brightness);
            setAllLEDs(red, green, blue);
            strip.show();
        }
        else
        {
            // Fade completed
            fading = false;
            currentBrightness = targetBrightness;
            strip.setBrightness(currentBrightness);
            setAllLEDs(red, green, blue);
            strip.show();
        }
    }

    void LEDManager::updateRainbowAnimation()
    {
        // Variabile statica per il timing del movimento dell'arcobaleno
        static unsigned long lastRainbowUpdate = 0;
        const uint8_t RAINBOW_SPEED_MS = 20; // 20ms tra ogni frame

        if (millis() - lastRainbowUpdate >= RAINBOW_SPEED_MS)
        {
            lastRainbowUpdate = millis();

            for (int i = 0; i < NUM_LEDS; i++)
            {
                // La formula chiave per l'arcobaleno scorrevole:
                // Combina la posizione del LED (i) e la posizione corrente di scorrimento (rainbowPosition)
                // L'offset assicura che il colore di ogni LED sia leggermente diverso
                uint32_t color = Wheel(((i * 256 / NUM_LEDS) + rainbowPosition) & 255);
                strip.setPixelColor(i, color);
            }

            strip.show();

            // Incrementa la posizione per il prossimo ciclo, creando l'effetto di scorrimento
            rainbowPosition++;
            if (rainbowPosition > 255)
            {
                rainbowPosition = 0; // Riparti da capo
            }
        }
    }

    // ============================================================================
    // THREAD-SAFE PUBLIC INTERFACE
    // ============================================================================

    void LEDManager::setLoadingState(bool on)
    {
        LEDEvent event;
        event.type = LEDEventType::SET_LOADING;
        event.state = on;
        sendLEDEvent(event);
    }

    void LEDManager::setRainbowState(bool on, uint8_t wait_ms)
    {
        LEDEvent event;
        event.type = LEDEventType::SET_RAINBOW;
        event.state = on;
        // Il parametro wait_ms non viene usato nell'evento, ma è utile
        // per documentare la velocità desiderata (che è fissa in updateRainbowAnimation)
        sendLEDEvent(event);
    }

    void LEDManager::flashColor(uint8_t r, uint8_t g, uint8_t b, int brightness, int duration)
    {
        LEDEvent event;
        event.type = LEDEventType::FLASH_COLOR;
        event.r = r;
        event.g = g;
        event.b = b;
        event.brightness = brightness;
        event.duration = duration;
        sendLEDEvent(event);
    }

    void LEDManager::activate()
    {
        LEDEvent event;
        event.type = LEDEventType::ACTIVATE;
        sendLEDEvent(event);
    }

    void LEDManager::updateLastEncoderMovementTime()
    {
        activate(); // Legacy compatibility
    }

    void LEDManager::setMainColor(const String &colorName)
    {
        PreferencesManager prefs;
        String actualColorName = colorName.isEmpty() ? prefs.get("conf.ledColor") : colorName;

        // Color palette mapping
        uint8_t r = 0, g = 181, b = 214; // Default azure

        if (actualColorName == "azure")
        {
            r = 0;
            g = 181;
            b = 214;
        }
        else if (actualColorName == "green")
        {
            r = 30;
            g = 254;
            b = 30;
        }
        else if (actualColorName == "red")
        {
            r = 255;
            g = 0;
            b = 0;
        }
        else if (actualColorName == "orange")
        {
            r = 254;
            g = 94;
            b = 0;
        }
        else if (actualColorName == "yellow")
        {
            r = 128;
            g = 128;
            b = 0;
        }
        else if (actualColorName == "blue")
        {
            r = 18;
            g = 0;
            b = 213;
        }
        else if (actualColorName == "violet")
        {
            r = 110;
            g = 0;
            b = 255;
        }
        else if (actualColorName == "purple")
        {
            r = 211;
            g = 0;
            b = 164;
        }
        else
        {
            SDK_LOGGER("⚠️ Unknown LED color: %s, using azure\n", actualColorName.c_str());
        }

        LEDEvent event;
        event.type = LEDEventType::SET_COLOR;
        event.r = r;
        event.g = g;
        event.b = b;
        sendLEDEvent(event);

        SDK_LOGGER("💡 LED Color set to: %s (%d,%d,%d)\n", actualColorName.c_str(), r, g, b);
    }

    // ============================================================================
    // HELPER FUNCTIONS
    // ============================================================================

    bool LEDManager::sendLEDEvent(const LEDEvent &event)
    {
        if (!ledEventQueue)
            return false;

        if (xQueueSend(ledEventQueue, &event, pdMS_TO_TICKS(10)) != pdPASS)
        {
            SDK_LOGGER("⚠️ LED event queue full!");
            return false;
        }
        return true;
    }

    void LEDManager::resetAllLEDs()
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }

    void LEDManager::setAllLEDs(uint8_t r, uint8_t g, uint8_t b)
    {
        red = r;
        green = g;
        blue = b;

        for (int i = 0; i < NUM_LEDS; i++)
        {
            strip.setPixelColor(i, strip.Color(r, g, b));
        }
    }

    void LEDManager::fadeToBrightness(int brightness, int duration)
    {
        startBrightness = currentBrightness;
        targetBrightness = brightness;
        if (!fading)
        {
            fadeStartMillis = millis();
        }
        fadeDuration = duration;
        fading = true;
    }

    void LEDManager::restartAnimationTask()
    {
        if (animationTaskHandle)
        {
            vTaskDelete(animationTaskHandle);
            animationTaskHandle = nullptr;
            SDK_LOGGER("💡 LED Animation task deleted");
        }

        delay(100); // Brief pause for cleanup
        startAnimationTask();
    }

    uint32_t LEDManager::Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if (WheelPos < 85)
        {
            // Rosso -> Verde
            return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if (WheelPos < 170)
        {
            // Verde -> Blu
            WheelPos -= 85;
            return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            // Blu -> Rosso
            WheelPos -= 170;
            return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }

} // namespace CloudMouse