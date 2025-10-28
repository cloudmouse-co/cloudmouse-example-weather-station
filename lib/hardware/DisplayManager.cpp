#include "./DisplayManager.h"
#include "../core/EventBus.h"

namespace CloudMouse::Hardware
{
    using namespace CloudMouse;

    // ============================================================================
    // LVGL: static variables definition for double buffer
    // ============================================================================

    lv_color_t *DisplayManager::buf1 = nullptr;
    lv_color_t *DisplayManager::buf2 = nullptr;

    // ============================================================================
    // CONSTRUCTOR AND DESTRUCTOR IMPLEMENTATION
    // ============================================================================

    DisplayManager::DisplayManager() : disp(nullptr), indev(nullptr), weatherUI(nullptr) { }

    DisplayManager::~DisplayManager()
    {
        // detaching weatherUI
        if (weatherUI) {
            delete weatherUI;
            weatherUI = nullptr;
        }

        if (buf1)
        {
            free(buf1); 
            buf1 = nullptr;
        }
        if (buf2)
        {
            free(buf2);
            buf2 = nullptr;
        }

        if (indev) lv_indev_delete(indev);
        if (disp) lv_display_delete(disp);
        
        lvgl_ticker.detach(); 

        lv_deinit();
    }

    // ============================================================================
    // SYSTEM LIFECYCLE IMPLEMENTATION
    // ============================================================================

    void DisplayManager::init()
    {
        Serial.println("🖥️ Initializing DisplayManager con LVGL v9...");

        display.init();
        display.setBrightness(200);

        lv_init();
        lvgl_ticker.attach_ms(4, lv_tick_task);

        const size_t bufSize = 480 * 32; 
        buf1 = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * bufSize);
        buf2 = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * bufSize);

        if (!buf1 || !buf2)
        {
            Serial.println("❌ LVGL in PSRAM buffer failed!");
            return;
        }
        Serial.printf("✅ Buffer LVGL allocated in PSRAM (2x %d bytes)\n", sizeof(lv_color_t) * bufSize);


        // 5. flushing PSRAM buffers to prevent residual corrupted data on power disconnection
        const size_t totalBufBytes = sizeof(lv_color_t) * bufSize;

        memset(buf1, 0, totalBufBytes);
        memset(buf2, 0, totalBufBytes);

        Serial.println("✅ PSRAM Buffers flushed.");

        // 6. LVGL display driver init (v9)
        disp = lv_display_create(getWidth(), getHeight());
        if (disp == NULL) {
             Serial.println("❌ LVGL display creation failed!");
             return;
        }
        lv_display_set_flush_cb(disp, lvgl_flush_cb);
        lv_display_set_buffers(disp, buf1, buf2, bufSize * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_user_data(disp, this);

        // 7. LVGL input (Encoder) driver init (v9)
        indev = lv_indev_create();
        if (indev == NULL) {
             Serial.println("❌ LVGL indev init failed!");
             return;
        }
        lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
        lv_indev_set_read_cb(indev, lvgl_encoder_read_cb);
        lv_indev_set_user_data(indev, this);

        // 8. Create a group and assing it to the encoder
        encoder_group = lv_group_create();
        lv_group_set_default(encoder_group);
        lv_indev_set_group(indev, encoder_group);

        // 9. Create LVGL UI 
        Serial.println("🎨 Creating UI LVGL...");
        createUi();

        initialized = true;
        Serial.printf("✅ DisplayManager with LVGL v9 succesfully initialized!\n");
    }

    void DisplayManager::update()
    {
        Event event;
        while (EventBus::instance().receiveFromMain(event, 0))
        {
            processEvent(event);
        }
        lv_timer_handler();
    }

        // Update time display on weather screen every second
        static unsigned long lastTimeUpdate = 0;
        if (weatherUI && millis() - lastTimeUpdate >= 1000) {
            weatherUI->updateTime();
            lastTimeUpdate = millis();
        }

        lv_timer_handler();
    }

    // ============================================================================
    // LVGL: "Glue" Callback Implementations (v9)
    // ============================================================================

    void DisplayManager::lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
    {
        DisplayManager *self = (DisplayManager *)lv_display_get_user_data(disp);
        if (!self) return;

        uint32_t w = lv_area_get_width(area);
        uint32_t h = lv_area_get_height(area);

        self->display.pushImage(area->x1, area->y1, w, h, (uint16_t *)px_map);

        lv_display_flush_ready(disp);
    }

    void DisplayManager::lvgl_encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
    {
        DisplayManager *self = (DisplayManager *)lv_indev_get_user_data(indev); // Corretto
        if (!self) return;

        data->enc_diff = self->encoder_diff;
        data->state = self->encoder_state;

        self->encoder_diff = 0;
        if (self->encoder_state == LV_INDEV_STATE_PRESSED)
        {
            self->encoder_state = LV_INDEV_STATE_RELEASED;
        }
    }

    // ============================================================================
    // EVENT PROCESSING IMPLEMENTATION
    // ============================================================================

    void DisplayManager::processEvent(const Event &event)
    {
        switch (event.type)
        {
        case EventType::DISPLAY_WAKE_UP:
            // currentScreen = Screen::HELLO_WORLD;
            // lv_disp_load_scr(screen_hello_world);
            break;

        case EventType::DISPLAY_WIFI_CONNECTING:
            currentScreen = Screen::WIFI_CONNECTING;
            lv_disp_load_scr(screen_wifi_connecting);
            break;

        case EventType::ENCODER_ROTATION:
            Serial.println(event.value);

            encoder_diff += event.value; 
            if (currentScreen == Screen::HELLO_WORLD) {
                lv_label_set_text_fmt(label_hello_status, "Encoder rotation: %s", event.value > 0 ? "RIGHT" : "LEFT");
            }
            break;

        case EventType::ENCODER_CLICK:
            encoder_state = LV_INDEV_STATE_PRESSED;
            if (currentScreen == Screen::HELLO_WORLD) {
                lv_label_set_text(label_hello_status, "Click!");
            }
            break;

        case EventType::ENCODER_LONG_PRESS:
            encoder_state = LV_INDEV_STATE_PRESSED; 
            if (currentScreen == Screen::HELLO_WORLD) {
                lv_label_set_text(label_hello_status, "Long Press!");
            }
            break;

        case EventType::DISPLAY_WIFI_AP_MODE:
            currentScreen = Screen::WIFI_AP_MODE;
            { 
                String apSSID = GET_AP_SSID();
                String apPassword = GET_AP_PASSWORD();
                String qrData = String("WIFI:T:WPA;S:") + apSSID + ";P:" + apPassword + ";;";
                
                lv_label_set_text(label_ap_mode_ssid, apSSID.c_str());
                lv_label_set_text(label_ap_mode_pass, apPassword.c_str());
                lv_qrcode_set_data(qr_ap_mode, qrData.c_str());
            }
            lv_disp_load_scr(screen_ap_mode);
            break;

        case EventType::DISPLAY_WIFI_SETUP_URL:
            currentScreen = Screen::WIFI_AP_CONNECTED;
            
            lv_qrcode_set_data(qr_ap_connected, WIFI_CONFIG_SERVICE);
            lv_label_set_text(label_ap_connected_url, WIFI_CONFIG_SERVICE);
            lv_disp_load_scr(screen_ap_connected);
            break;

        case EventType::DISPLAY_CLEAR:
            lv_obj_clean(lv_screen_active()); 
            break;

        case EventType::DISPLAY_UPDATE:
            // Weather data update received from Core
            if (weatherUI) {
                // The WeatherService will be accessed via Core
                // For now, just show the weather screen
                weatherUI->show();
            }
            break;

        case EventType::WEATHER_DATA_CURRENT:
        {
            if (!weatherUI) break;
            

            // Parse: "temp|humidity|windSpeed|weatherCode|condition"
            String data = String(event.stringData);
            Serial.println(data);
            int idx1 = data.indexOf('|');
            int idx2 = data.indexOf('|', idx1 + 1);
            int idx3 = data.indexOf('|', idx2 + 1);
            int idx4 = data.indexOf('|', idx3 + 1);
            
            WeatherData weather;
            weather.temperature = data.substring(0, idx1).toFloat();
            weather.humidity = data.substring(idx1 + 1, idx2).toInt();
            weather.windSpeed = data.substring(idx2 + 1, idx3).toFloat();
            weather.weatherCode = data.substring(idx3 + 1, idx4).toInt();
            weather.condition = data.substring(idx4 + 1);
            weather.isValid = true;
            
            weatherUI->updateCurrentWeather(weather);
            weatherUI->show();
            
            Serial.println("✅ Current weather UI updated");
        }
        break;

        case EventType::WEATHER_DATA_FORECAST:
        {
            if (!weatherUI) break;
            
            int dayIndex = event.value;
            
            // Parse: "date|tempMax|tempMin|weatherCode"
            String data = String(event.stringData);
            int idx1 = data.indexOf('|');
            int idx2 = data.indexOf('|', idx1 + 1);
            int idx3 = data.indexOf('|', idx2 + 1);
            
            DayForecast forecast;
            forecast.date = data.substring(0, idx1);
            forecast.tempMax = data.substring(idx1 + 1, idx2).toFloat();
            forecast.tempMin = data.substring(idx2 + 1, idx3).toFloat();
            forecast.weatherCode = data.substring(idx3 + 1).toInt();
            
            // Store it temporarily (you'll need an array in DisplayManager)
            forecastData[dayIndex] = forecast;
            
            // If it's the last forecast day, update the UI
            if (dayIndex == 2) {
                weatherUI->updateForecast(forecastData);
                Serial.println("✅ Forecast UI updated");
            }
        }
        break;

        default:
            break;
        }
    }

    // ============================================================================
    // LVGL: UI Creation Methods (v9)
    // ============================================================================

    lv_obj_t* DisplayManager::createHeader(lv_obj_t* parent, const char* title) {
        lv_obj_t* header = lv_obj_create(parent);
        lv_obj_set_size(header, 480, 40);
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(header, lv_color_hex(0x222222), 0);
        lv_obj_set_style_border_width(header, 0, 0);
        lv_obj_set_style_radius(header, 0, 0);

        lv_obj_t* label = lv_label_create(header);
        lv_label_set_text(label, title);
        lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_center(label);
        
        return header;
    }

    void DisplayManager::createUi()
    {
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(COLOR_BG), 0); 

        createHelloWorldScreen();
        createWifiConnectingScreen();
        createApModeScreen();
        createApConnectedScreen();

        weatherUI = new WeatherUI();
        weatherUI->createScreen();
    }

    void DisplayManager::createHelloWorldScreen()
    {
        screen_hello_world = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen_hello_world, lv_color_hex(COLOR_BG), 0);
        createHeader(screen_hello_world, "CloudMouse Boilerplate");

        lv_obj_t* title = lv_label_create(screen_hello_world);
        lv_label_set_text(title, "Hello CloudMouse!");
        lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

        label_hello_status = lv_label_create(screen_hello_world);
        lv_label_set_text(label_hello_status, "Ready!");
        lv_obj_set_style_text_color(label_hello_status, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_set_style_text_font(label_hello_status, &lv_font_montserrat_20, 0);
        lv_obj_align(label_hello_status, LV_ALIGN_CENTER, 0, 20);
        
        lv_obj_t* instructions = lv_label_create(screen_hello_world);
        lv_label_set_text(instructions, "Rotate the knob or push the button");
        lv_obj_set_style_text_color(instructions, lv_color_hex(0x888888), 0);
        lv_obj_align(instructions, LV_ALIGN_BOTTOM_MID, 0, -20);
    }

    void DisplayManager::createWifiConnectingScreen()
    {
        screen_wifi_connecting = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen_wifi_connecting, lv_color_hex(COLOR_BG), 0);
        createHeader(screen_wifi_connecting, "CloudMouse Boilerplate");

        lv_obj_t* title = lv_label_create(screen_wifi_connecting);
        lv_label_set_text(title, "Connecting to WiFi");
        lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

        label_wifi_status = lv_label_create(screen_wifi_connecting);
        lv_label_set_text(label_wifi_status, "Please wait...");
        lv_obj_set_style_text_color(label_wifi_status, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_set_style_text_font(label_wifi_status, &lv_font_montserrat_20, 0);
        lv_obj_align(label_wifi_status, LV_ALIGN_CENTER, 0, 20);

        spinner_wifi = lv_spinner_create(screen_wifi_connecting);
        lv_obj_set_size(spinner_wifi, 64, 64);
        lv_obj_align(spinner_wifi, LV_ALIGN_CENTER, 0, 80);
        lv_obj_set_style_arc_color(spinner_wifi, lv_color_hex(COLOR_ACCENT), LV_PART_INDICATOR);
    }

    void DisplayManager::createApModeScreen()
    {
        screen_ap_mode = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen_ap_mode, lv_color_hex(TFT_DARKGRAY), 0);
        createHeader(screen_ap_mode, "WiFi Setup Required");

        lv_obj_t* title = lv_label_create(screen_ap_mode);
        lv_label_set_text(title, "Connect to CloudMouse");
        lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);

        label_ap_mode_ssid = lv_label_create(screen_ap_mode);
        lv_label_set_text(label_ap_mode_ssid, "SSID: ...");
        lv_obj_set_style_text_color(label_ap_mode_ssid, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_align(label_ap_mode_ssid, LV_ALIGN_TOP_MID, 0, 90);

        label_ap_mode_pass = lv_label_create(screen_ap_mode);
        lv_label_set_text(label_ap_mode_pass, "Pass: ...");
        lv_obj_set_style_text_color(label_ap_mode_pass, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_align(label_ap_mode_pass, LV_ALIGN_TOP_MID, 0, 110);

        qr_ap_mode = lv_qrcode_create(screen_ap_mode);
        lv_obj_set_size(qr_ap_mode, 180, 180); 
        lv_qrcode_set_dark_color(qr_ap_mode, lv_color_hex(0x000000));
        lv_qrcode_set_light_color(qr_ap_mode, lv_color_hex(0xFFFFFF));
        
        lv_qrcode_set_data(qr_ap_mode, "WIFI:T:WPA;S:...;P:...;;"); 
        lv_obj_align(qr_ap_mode, LV_ALIGN_CENTER, 0, 40);
    }

    void DisplayManager::createApConnectedScreen()
    {
        screen_ap_connected = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screen_ap_connected, lv_color_hex(TFT_DARKGREEN), 0);
        createHeader(screen_ap_connected, "WiFi Configuration");

        lv_obj_t* title = lv_label_create(screen_ap_connected);
        lv_label_set_text(title, "✅ Connected!");
        lv_obj_set_style_text_color(title, lv_color_hex(COLOR_SUCCESS), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);
        
        lv_obj_t* subtitle = lv_label_create(screen_ap_connected);
        lv_label_set_text(subtitle, "Scan QR to setup WiFi");
        lv_obj_set_style_text_color(subtitle, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 90);

        qr_ap_connected = lv_qrcode_create(screen_ap_connected);
        lv_obj_set_size(qr_ap_connected, 180, 180); 
        lv_qrcode_set_dark_color(qr_ap_connected, lv_color_hex(0x000000));
        lv_qrcode_set_light_color(qr_ap_connected, lv_color_hex(0xFFFFFF));
        
        lv_qrcode_set_data(qr_ap_connected, "http://..."); 
        lv_obj_align(qr_ap_connected, LV_ALIGN_CENTER, 0, 30);
        
        label_ap_connected_url = lv_label_create(screen_ap_connected);
        lv_label_set_text(label_ap_connected_url, "http://..."); 
        lv_obj_set_style_text_color(label_ap_connected_url, lv_color_hex(COLOR_TEXT), 0);
        lv_obj_align(label_ap_connected_url, LV_ALIGN_BOTTOM_MID, 0, -20);
    }
} // namespace CloudMouse::Hardware
