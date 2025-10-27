/**
 * WeatherService - OpenMeteo API Integration
 * 
 * Business logic service that handles:
 * - Weather data fetching from Open-Meteo API
 * - JSON parsing and data validation
 * - Periodic weather updates (15 minutes)
 * - Event emission for UI updates
 * 
 * This service runs on Core 0 and communicates with DisplayManager via events
 */

#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace CloudMouse {

/**
 * Weather condition interpretation from WMO codes
 */
struct WeatherData {
    float temperature;
    int humidity;
    float windSpeed;
    String condition;
    int weatherCode;
    bool isValid;
    
    WeatherData() : temperature(0), humidity(0), windSpeed(0), 
                    weatherCode(0), isValid(false) {}
};

/**
 * Daily forecast data structure
 */
struct DayForecast {
    String date;         // ISO format: YYYY-MM-DD
    float tempMax;
    float tempMin;
    int weatherCode;
    
    DayForecast() : tempMax(0), tempMin(0), weatherCode(0) {}
};

/**
 * WeatherService - Core weather business logic
 * 
 * Responsibilities:
 * - Fetch weather data from Open-Meteo API
 * - Parse JSON responses
 * - Validate and store weather data
 * - Emit events to notify UI of updates
 * - Handle periodic refresh (15 min intervals)
 */
class WeatherService {
public:
    WeatherService(float lat, float lon);
    
    // Lifecycle
    void begin();
    void update();
    
    // Data access
    const WeatherData& getCurrentWeather() const { return currentWeather; }
    const DayForecast* getForecast() const { return forecast; }
    bool isForecastValid() const { return forecastValid; }
    
    // Manual refresh
    bool fetchWeather();
    
    // Configuration
    void setUpdateInterval(unsigned long interval) { updateInterval = interval; }
    unsigned long getUpdateInterval() const { return updateInterval; }
    
private:
    // Configuration
    float latitude;
    float longitude;
    unsigned long updateInterval;
    unsigned long lastUpdate;
    
    // Weather data
    WeatherData currentWeather;
    DayForecast forecast[3];
    bool forecastValid;
    
    // Helper methods
    String getWeatherCondition(int code);
    void notifyWeatherUpdate();
};

} // namespace CloudMouse
