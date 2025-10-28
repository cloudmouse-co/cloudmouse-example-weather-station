#pragma once

#include <Arduino.h>
#include <Ticker.h>
#include <lvgl.h>
#include "LGFX_ILI9488.h"
#include "../core/Events.h"
#include "../config/DeviceConfig.h"
#include "../config/lv_conf.h"
#include "../ui/WeatherUI.h"

/**
 * Operational and idle brightness targets with dimmer fade out
 */
#define BRIGHTNESS_UP_TARGET 180
#define BRIGHTNESS_IDLE_TARGET 10
#define FADE_OUT_STEP_DELAY_MS 20
#define FADE_OUT_STEP_VALUE 2

/**
 * Display Management Controller
 *
 * High-level display controller providing event-driven UI management with multiple
 * screen states, smooth animations, and integrated QR code generation capabilities.
 *
 * Architecture:
 * - Event-driven screen state machine with automatic transitions
 * - Hardware-accelerated sprite rendering with PSRAM buffering
 * - Modular screen rendering system for extensible UI development
 * - Integrated timing management for smooth animations and responsive updates
 *
 * Memory Management:
 * - Uses PSRAM for large sprite buffer to preserve main RAM
 * - Automatic sprite allocation and cleanup in constructor/destructor
 * - Efficient color palette with predefined UI theme colors
 * - Memory-conscious rendering with selective update regions
 *
 * Thread Safety:
 * - Designed for single UI task usage (typically Core1)
 * - Event processing is non-blocking with queue-based communication
 * - Sprite operations are inherently thread-safe when called from single task
 * - No shared mutable state across multiple tasks
 */

namespace CloudMouse::Hardware
{
    using namespace CloudMouse::Utils;

    class DisplayManager
    {
    public:
        DisplayManager();
        ~DisplayManager();

        void init();
        void update();
        void processEvent(const CloudMouse::Event &event);
        
        bool isReady() const { return initialized; }
        int getWidth() const { return 480; }
        int getHeight() const { return 320; }
        bool isAnimating() const { return initialized; }

    private:

        WeatherUI* weatherUI = nullptr;
        DayForecast forecastData[3];

        enum class Screen
        {
            HELLO_WORLD,
            WIFI_CONNECTING,
            WIFI_AP_MODE,
            WIFI_AP_CONNECTED
        };

        LGFX_ILI9488 display; 

        // ========================================================================
        // LVGL DRIVER & BUFFER & TICKER
        // ========================================================================

        lv_display_t * disp;      
        lv_indev_t * indev;

        static lv_color_t *buf1;
        static lv_color_t *buf2;

        // Signature callbacks
        static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
        static void lvgl_encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

        Ticker lvgl_ticker;
        static void lv_tick_task() { lv_tick_inc(5); }

        // ========================================================================
        // LVGL UI OBJECTS
        // ========================================================================

        lv_group_t *encoder_group; 
        
        lv_obj_t *screen_hello_world;
        lv_obj_t *screen_wifi_connecting;
        lv_obj_t *screen_ap_mode;
        lv_obj_t *screen_ap_connected;

        lv_obj_t *label_hello_status;
        lv_obj_t *spinner_wifi;
        lv_obj_t *label_wifi_status;
        lv_obj_t *qr_ap_mode;
        lv_obj_t *qr_ap_connected;
        lv_obj_t *label_ap_connected_url;
        lv_obj_t *label_ap_mode_ssid;
        lv_obj_t *label_ap_mode_pass;

        // ========================================================================
        // STATE MANAGEMENT VARIABLES
        // ========================================================================

        bool initialized = false;
        Screen currentScreen = Screen::HELLO_WORLD;
        int32_t encoder_diff = 0;
        lv_indev_state_t encoder_state = LV_INDEV_STATE_RELEASED;

        // Brightness management variables
        int currentBrightness = BRIGHTNESS_UP_TARGET;
        unsigned long lastInteractionTime = 0;
        unsigned long lastFadeTime = 0;

        // ========================================================================
        // UI COLOR SCHEME DEFINITIONS
        // ========================================================================

        static const uint16_t COLOR_BG = 0x0000;
        static const uint16_t COLOR_TEXT = 0xFFFF;
        static const uint16_t COLOR_ACCENT = 0x07FF;
        static const uint16_t COLOR_SUCCESS = 0x07E0;
        static const uint16_t COLOR_WARNING = 0xFD20;

        // ========================================================================
        // SCREEN UI CREATION METHODS
        // ========================================================================

        void createUi();
        void createHelloWorldScreen();
        void createWifiConnectingScreen();
        void createApModeScreen();
        void createApConnectedScreen();
        lv_obj_t* createHeader(lv_obj_t* parent, const char* title);

        // ========================================================================
        // SCREEN BRIGHTNESS MANAGEMENT
        // ========================================================================
        
        void wakeUp();
        void handleDimmer();
    };
};