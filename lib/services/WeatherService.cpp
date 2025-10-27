/**
 * WeatherService Implementation
 * 
 * Core weather business logic separated from UI concerns.
 * Runs on Core 0, communicates with UI via EventBus.
 */

#include "WeatherService.h"
#include "../core/EventBus.h"
#include "../core/Events.h"

namespace CloudMouse {

WeatherService::WeatherService(float lat, float lon) 
    : latitude(lat), 
      longitude(lon),
      updateInterval(900000), // 15 minutes default
      lastUpdate(0),
      forecastValid(false) {
}

void WeatherService::begin() {
    Serial.println("🌤️ WeatherService initializing...");
    
    // Initial fetch
    if (fetchWeather()) {
        Serial.println("✅ Initial weather data fetched");
        notifyWeatherUpdate();
    } else {
        Serial.println("⚠️ Failed to fetch initial weather data");
    }
    
    lastUpdate = millis();
}

void WeatherService::update() {
    // Check if it's time to refresh
    if (millis() - lastUpdate >= updateInterval) {
        Serial.println("🔄 Updating weather data...");
        
        if (fetchWeather()) {
            Serial.println("✅ Weather data updated");
            notifyWeatherUpdate();
        } else {
            Serial.println("⚠️ Weather update failed");
        }
        
        lastUpdate = millis();
    }
}

bool WeatherService::fetchWeather() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi not connected!");
        return false;
    }
    
    // Build Open-Meteo API URL
    String url = "http://api.open-meteo.com/v1/forecast?";
    url += "latitude=" + String(latitude, 4);
    url += "&longitude=" + String(longitude, 4);
    url += "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m";
    url += "&daily=weather_code,temperature_2m_max,temperature_2m_min";
    url += "&timezone=auto";
    url += "&forecast_days=4";
    
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != 200) {
        Serial.printf("❌ HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.printf("❌ JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    // Parse current weather
    JsonObject current = doc["current"];
    if (!current.isNull()) {
        currentWeather.temperature = current["temperature_2m"];
        currentWeather.humidity = current["relative_humidity_2m"];
        currentWeather.windSpeed = current["wind_speed_10m"];
        currentWeather.weatherCode = current["weather_code"];
        currentWeather.condition = getWeatherCondition(currentWeather.weatherCode);
        currentWeather.isValid = true;
        
        Serial.printf("📊 Current: %.1f°C, %d%%, %.1f km/h\n", 
                      currentWeather.temperature, 
                      currentWeather.humidity, 
                      currentWeather.windSpeed);
    }
    
    // Parse forecast (skip today, get next 3 days)
    JsonObject daily = doc["daily"];
    if (!daily.isNull()) {
        JsonArray dates = daily["time"];
        JsonArray maxTemps = daily["temperature_2m_max"];
        JsonArray minTemps = daily["temperature_2m_min"];
        JsonArray codes = daily["weather_code"];
        
        for (int i = 0; i < 3 && i + 1 < dates.size(); i++) {
            forecast[i].date = dates[i + 1].as<String>();
            forecast[i].tempMax = maxTemps[i + 1];
            forecast[i].tempMin = minTemps[i + 1];
            forecast[i].weatherCode = codes[i + 1];
        }
        forecastValid = true;
    }
    
    notifyWeatherUpdate();

    return true;
}

String WeatherService::getWeatherCondition(int code) {
    if (code == 0) return "Clear Sky";
    if (code <= 3) return "Cloud Sun";
    if (code <= 8) return "Cloudy";
    if (code <= 48) return "Foggy";
    if (code <= 67) return "Rainy";
    if (code <= 77) return "Snowy";
    if (code <= 82) return "Showers";
    if (code <= 86) return "Snow Showers";
    if (code <= 99) return "Thunderstorm";
    return "Unknown";
}

void WeatherService::notifyWeatherUpdate() {
    // Send event to UI with updated weather data
    Event weatherEvent(EventType::WEATHER_DATA_CURRENT);
    String data = String(currentWeather.temperature, 1) + "|" +
                  String(currentWeather.humidity) + "|" +
                  String(currentWeather.windSpeed, 1) + "|" +
                  String(currentWeather.weatherCode) + "|" +
                  currentWeather.condition;
    weatherEvent.setStringData(data.c_str());
    EventBus::instance().sendToUI(weatherEvent);
    
    // Pack forecast data
    if (forecastValid) {
        for (int i = 0; i < 3; i++) {
            Event forecastEvent(EventType::WEATHER_DATA_FORECAST);
            forecastEvent.value = i;
            String forecastData = forecast[i].date + "|" +
                                  String(forecast[i].tempMax, 1) + "|" +
                                  String(forecast[i].tempMin, 1) + "|" +
                                  String(forecast[i].weatherCode);
            forecastEvent.setStringData(forecastData.c_str());
            EventBus::instance().sendToUI(forecastEvent);
        }
    }
    
    Serial.println("📤 Weather data sent to UI via events");
}

} // namespace CloudMouse
