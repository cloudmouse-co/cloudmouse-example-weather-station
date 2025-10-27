/**
 * WeatherUI - LVGL Weather Display Component
 * 
 * Handles all weather-related UI rendering using LVGL widgets.
 * Separated from DisplayManager to keep concerns clean and modular.
 * 
 * This component:
 * - Creates weather-specific LVGL screens and widgets
 * - Updates weather display with current data
 * - Formats weather information for display
 * - Manages Font Awesome weather icons
 */

#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "../services/WeatherService.h"

namespace CloudMouse {

// Font Awesome icon definitions for weather
#define FA_SUN                "\xEF\x86\x85"
#define FA_SUN_CLOUD          "\xEF\x9B\x84"
#define FA_CLOUD              "\xEF\x83\x82"
#define FA_CLOUD_RAIN         "\xEF\x9D\x80"
#define FA_SNOWFLAKE          "\xEF\x8B\x9C"
#define FA_BOLT_CLOUD         "\xEF\x9D\xAC"
#define FA_SMOG               "\xEF\x9D\xA9"

// Declare Font Awesome fonts (must be included in your project)
extern "C" {
  LV_FONT_DECLARE(font_awesome_solid_48);
  LV_FONT_DECLARE(font_awesome_solid_24);
}

/**
 * WeatherUI - Weather display component for LVGL
 * 
 * Responsibilities:
 * - Create weather screen layout
 * - Render current weather information
 * - Display 3-day forecast
 * - Update time display
 * - Format weather data for display
 */
class WeatherUI {
public:
    WeatherUI();
    ~WeatherUI();
    
    // Screen lifecycle
    void createScreen();
    void show();
    void hide();
    
    // Data updates
    void updateCurrentWeather(const WeatherData& weather);
    void updateForecast(const DayForecast* forecast);
    void updateTime();
    
    // Widget access
    lv_obj_t* getScreen() const { return screen; }
    
private:
    // Main screen
    lv_obj_t* screen;
    
    // Current weather widgets
    lv_obj_t* dateLabelDay;
    lv_obj_t* dateLabelDate;
    lv_obj_t* timeLabel;
    lv_obj_t* weatherIcon;
    lv_obj_t* tempLabel;
    lv_obj_t* conditionLabel;
    lv_obj_t* humidityLabel;
    lv_obj_t* windLabel;
    
    // Forecast widgets
    lv_obj_t* forecastDay1Container;
    lv_obj_t* forecastDay2Container;
    lv_obj_t* forecastDay3Container;
    
    // Helper methods
    void createTopBar();
    void createBottomBar();
    void createForecastDay(lv_obj_t* container, const DayForecast& forecast);
    
    const char* getWeatherIconFA(int code);
    String formatForecastDate(const String& isoDate);
};

} // namespace CloudMouse
