/**
 * WeatherUI Implementation
 * 
 * LVGL-based weather display component with pixel-perfect UI.
 * All UI logic separated from business logic for clean architecture.
 */

#include "WeatherUI.h"
#include <time.h>

namespace CloudMouse {

WeatherUI::WeatherUI() 
    : screen(nullptr),
      dateLabelDay(nullptr),
      dateLabelDate(nullptr),
      timeLabel(nullptr),
      weatherIcon(nullptr),
      tempLabel(nullptr),
      conditionLabel(nullptr),
      humidityLabel(nullptr),
      windLabel(nullptr),
      forecastDay1Container(nullptr),
      forecastDay2Container(nullptr),
      forecastDay3Container(nullptr) {
}

WeatherUI::~WeatherUI() {
    if (screen) {
        lv_obj_del(screen);
    }
}

void WeatherUI::createScreen() {
    Serial.println("🎨 Creating Weather UI screen...");
    
    // Create main screen
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a1a1a), 0);
    
    createTopBar();
    createBottomBar();
    
    Serial.println("✅ Weather UI created");
}

void WeatherUI::createTopBar() {
    // ========== TOP BAR (200px) ==========
    lv_obj_t* topBar = lv_obj_create(screen);
    lv_obj_set_size(topBar, 480, 200);
    lv_obj_set_pos(topBar, 0, 0);
    lv_obj_set_style_bg_color(topBar, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(topBar, 0, 0);
    lv_obj_set_style_pad_all(topBar, 0, 0);
    
    // LEFT SIDE - Date and Time
    lv_obj_t* leftSide = lv_obj_create(topBar);
    lv_obj_set_size(leftSide, 240, 200);
    lv_obj_set_pos(leftSide, 0, 0);
    lv_obj_set_style_bg_color(leftSide, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(leftSide, 0, 0);
    lv_obj_set_style_pad_all(leftSide, 15, 0);
    
    // Date (day, month, date)
    dateLabelDay = lv_label_create(leftSide);
    lv_obj_set_style_text_font(dateLabelDay, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(dateLabelDay, lv_color_hex(0xCCCCCC), 0);
    lv_label_set_text(dateLabelDay, "Loading...");
    lv_obj_align(dateLabelDay, LV_ALIGN_TOP_LEFT, 0, 0);

    dateLabelDate = lv_label_create(leftSide);
    lv_obj_set_style_text_font(dateLabelDate, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(dateLabelDate, lv_color_hex(0x888888), 0);
    lv_label_set_text(dateLabelDate, "Loading...");
    lv_obj_align(dateLabelDate, LV_ALIGN_TOP_LEFT, 0, 30);
    
    // Time (HH:MM:SS)
    timeLabel = lv_label_create(leftSide);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(timeLabel, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(timeLabel, "--:--:--");
    lv_obj_align(timeLabel, LV_ALIGN_BOTTOM_LEFT, 0, -70);
    
    // RIGHT SIDE - Current Weather
    lv_obj_t* rightSide = lv_obj_create(topBar);
    lv_obj_set_size(rightSide, 240, 200);
    lv_obj_set_pos(rightSide, 240, 0);
    lv_obj_set_style_bg_color(rightSide, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(rightSide, 0, 0);
    lv_obj_set_style_pad_all(rightSide, 15, 0);
    
    // Weather Icon (top center)
    weatherIcon = lv_label_create(rightSide);
    lv_obj_set_style_text_font(weatherIcon, &font_awesome_solid_48, 0);
    lv_obj_set_style_text_color(weatherIcon, lv_color_hex(0xFFAA00), 0);
    lv_label_set_text(weatherIcon, FA_SUN);
    lv_obj_align(weatherIcon, LV_ALIGN_TOP_MID, 0, -5);
    
    // Temperature (below icon)
    tempLabel = lv_label_create(rightSide);
    lv_obj_set_style_text_font(tempLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(tempLabel, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(tempLabel, "--.-°");
    lv_obj_align(tempLabel, LV_ALIGN_TOP_MID, 0, 55);
    
    // Condition (below temperature)
    conditionLabel = lv_label_create(rightSide);
    lv_obj_set_style_text_font(conditionLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(conditionLabel, lv_color_hex(0xAAAAAA), 0);
    lv_label_set_text(conditionLabel, "Loading...");
    lv_obj_align(conditionLabel, LV_ALIGN_TOP_MID, 0, 110);
    
    // Humidity (bottom left)
    humidityLabel = lv_label_create(rightSide);
    lv_obj_set_style_text_font(humidityLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(humidityLabel, lv_color_hex(0x4DA6FF), 0);
    lv_label_set_text(humidityLabel, LV_SYMBOL_TINT " ---%");
    lv_obj_align(humidityLabel, LV_ALIGN_BOTTOM_MID, -16, 0);
    
    // Wind (bottom right)
    windLabel = lv_label_create(rightSide);
    lv_obj_set_style_text_font(windLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(windLabel, lv_color_hex(0x66FF99), 0);
    lv_label_set_text(windLabel, LV_SYMBOL_REFRESH " -- km/h");
    lv_obj_align(windLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void WeatherUI::createBottomBar() {
    // ========== BOTTOM BAR (120px) - Forecast ==========
    lv_obj_t* bottomBar = lv_obj_create(screen);
    lv_obj_set_size(bottomBar, 480, 120);
    lv_obj_set_pos(bottomBar, 0, 200);
    lv_obj_set_style_bg_color(bottomBar, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(bottomBar, 0, 0);
    lv_obj_set_style_pad_all(bottomBar, 0, 0);
    lv_obj_set_flex_flow(bottomBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottomBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create 3 forecast containers
    forecastDay1Container = lv_obj_create(bottomBar);
    lv_obj_set_size(forecastDay1Container, 145, 100);
    lv_obj_set_style_bg_color(forecastDay1Container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(forecastDay1Container, 0, 0);
    lv_obj_set_style_radius(forecastDay1Container, 10, 0);
    lv_obj_clear_flag(forecastDay1Container, LV_OBJ_FLAG_SCROLLABLE);
    
    forecastDay2Container = lv_obj_create(bottomBar);
    lv_obj_set_size(forecastDay2Container, 145, 100);
    lv_obj_set_style_bg_color(forecastDay2Container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(forecastDay2Container, 0, 0);
    lv_obj_set_style_radius(forecastDay2Container, 10, 0);
    lv_obj_clear_flag(forecastDay2Container, LV_OBJ_FLAG_SCROLLABLE);
    
    forecastDay3Container = lv_obj_create(bottomBar);
    lv_obj_set_size(forecastDay3Container, 145, 100);
    lv_obj_set_style_bg_color(forecastDay3Container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(forecastDay3Container, 0, 0);
    lv_obj_set_style_radius(forecastDay3Container, 10, 0);
    lv_obj_clear_flag(forecastDay3Container, LV_OBJ_FLAG_SCROLLABLE);
}

void WeatherUI::show() {
    if (screen) {
        lv_disp_load_scr(screen);
    }
}

void WeatherUI::hide() {
    // Screen cleanup if needed
}

void WeatherUI::updateCurrentWeather(const WeatherData& weather) {
    if (!weather.isValid) return;
    
    // Update temperature
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f°", weather.temperature);
    lv_label_set_text(tempLabel, tempStr);
    
    // Update condition
    lv_label_set_text(conditionLabel, weather.condition.c_str());
    
    // Update humidity
    char humidityStr[16];
    snprintf(humidityStr, sizeof(humidityStr), LV_SYMBOL_TINT " %d%%", weather.humidity);
    lv_label_set_text(humidityLabel, humidityStr);
    
    // Update wind
    char windStr[32];
    snprintf(windStr, sizeof(windStr), LV_SYMBOL_REFRESH " %.1f km/h", weather.windSpeed);
    lv_label_set_text(windLabel, windStr);
    
    // Update weather icon
    lv_label_set_text(weatherIcon, getWeatherIconFA(weather.weatherCode));
    
    Serial.println("✅ Current weather UI updated");
}

void WeatherUI::updateForecast(const DayForecast* forecast) {
    if (!forecast) return;
    
    createForecastDay(forecastDay1Container, forecast[0]);
    createForecastDay(forecastDay2Container, forecast[1]);
    createForecastDay(forecastDay3Container, forecast[2]);
    
    Serial.println("✅ Forecast UI updated");
}

void WeatherUI::createForecastDay(lv_obj_t* container, const DayForecast& forecast) {
    // Clear previous content
    lv_obj_clean(container);
    
    // Weather Icon
    lv_obj_t* icon = lv_label_create(container);
    lv_obj_set_style_text_font(icon, &font_awesome_solid_24, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0x999999), 0);
    lv_label_set_text(icon, getWeatherIconFA(forecast.weatherCode));
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, -2);
    
    // Temperature MAX (white)
    lv_obj_t* tempMax = lv_label_create(container);
    lv_obj_set_style_text_font(tempMax, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(tempMax, lv_color_hex(0xFFFFFF), 0);
    char tempMaxStr[16];
    snprintf(tempMaxStr, sizeof(tempMaxStr), "%.0f°", forecast.tempMax);
    lv_label_set_text(tempMax, tempMaxStr);
    lv_obj_align(tempMax, LV_ALIGN_TOP_LEFT, 28, 32);
    
    // Separator "/"
    lv_obj_t* separator = lv_label_create(container);
    lv_obj_set_style_text_font(separator, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(separator, lv_color_hex(0x888888), 0);
    lv_label_set_text(separator, "/");
    lv_obj_align_to(separator, tempMax, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    
    // Temperature MIN (gray)
    lv_obj_t* tempMin = lv_label_create(container);
    lv_obj_set_style_text_font(tempMin, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(tempMin, lv_color_hex(0x888888), 0);
    char tempMinStr[16];
    snprintf(tempMinStr, sizeof(tempMinStr), "%.0f°", forecast.tempMin);
    lv_label_set_text(tempMin, tempMinStr);
    lv_obj_align_to(tempMin, separator, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    
    // Date
    lv_obj_t* date = lv_label_create(container);
    lv_obj_set_style_text_font(date, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(date, lv_color_hex(0x666666), 0);
    lv_label_set_text(date, formatForecastDate(forecast.date).c_str());
    lv_obj_align(date, LV_ALIGN_BOTTOM_MID, 0, 8);
}

void WeatherUI::updateTime() {
    static char dateStrDay[64];
    static char dateStrDate[64];
    char timeStr[16];
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    if (timeinfo->tm_year > (2020 - 1900)) {
        strftime(dateStrDay, sizeof(dateStrDay), "%A", timeinfo);
        strftime(dateStrDate, sizeof(dateStrDate), "%B %d, %Y", timeinfo);
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
        
        lv_label_set_text(dateLabelDay, dateStrDay);
        lv_label_set_text(dateLabelDate, dateStrDate);
        lv_label_set_text(timeLabel, timeStr);
    }
}

const char* WeatherUI::getWeatherIconFA(int code) {
    if (code == 0) return FA_SUN;
    if (code <= 3) return FA_SUN_CLOUD;
    if (code <= 8) return FA_CLOUD;
    if (code <= 48) return FA_SMOG;
    if (code <= 67) return FA_CLOUD_RAIN;
    if (code <= 77) return FA_SNOWFLAKE;
    if (code <= 82) return FA_CLOUD_RAIN;
    if (code <= 86) return FA_SNOWFLAKE;
    if (code <= 99) return FA_BOLT_CLOUD;
    return FA_CLOUD;
}

String WeatherUI::formatForecastDate(const String& isoDate) {
    if (isoDate.length() < 10) return "---";
    
    // Parse date
    int year = isoDate.substring(0, 4).toInt();
    int month = isoDate.substring(5, 7).toInt();
    int day = isoDate.substring(8, 10).toInt();
    
    // Create tm struct
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = 12; // Set to noon for correct day calculation
    
    // Calculate day of week
    mktime(&timeinfo);
    
    // Format as "27 Mon"
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%d %a", &timeinfo);
    return String(buffer);
}

} // namespace CloudMouse
