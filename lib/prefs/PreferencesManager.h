/**
 * CloudMouse SDK - Preferences Manager
 *
 * Handles persistent storage of configuration settings using ESP32 NVS.
 * Provides thread-safe access to WiFi credentials, device settings, and generic key-value storage.
 *
 * Features:
 * - Automatic initialization with default device settings
 * - WiFi credential management
 * - Generic key-value storage interface
 * - Safe clear/reset operations
 */

#ifndef PREFERENCESMANAGER_H
#define PREFERENCESMANAGER_H

#include <Preferences.h>

namespace CloudMouse::Prefs
{
    class PreferencesManager
    {
    public:
        // System lifecycle
        void init(); // Initialize with default settings

        // WiFi credential management
        bool hasWiFiCredentials(); // Check if WiFi credentials are stored
        void saveWiFiCredentials(const String &ssid, const String &password);
        String getWiFiSSID();     // Get stored WiFi SSID
        String getWiFiPassword(); // Get stored WiFi password

        // Generic storage interface
        void save(const char *key, const String &value); // Store any key-value pair
        String get(const char *key);                     // Retrieve value by key

        // Reset operations
        void clear();    // Clear current namespace
        void clearAll(); // Clear all stored preferences

    private:
        Preferences preferences; // ESP32 NVS interface

        // Configuration
        const char *space = "my-app"; // NVS namespace

        // Default values for device settings
        const int DEFAULT_BRIGHTNESS = 80;
        const String DEFAULT_LED_COLOR = "azure";
        const String DEFAULT_LANGUAGE = "it";
        const String DEFAULT_THEME = "light";

        // Internal methods
        void begin(bool readOnly); // Open preferences namespace
        void end();                // Close preferences and release resources
        void initDeviceSettings(); // Initialize default configuration values
    };
};
#endif