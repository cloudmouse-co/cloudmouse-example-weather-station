/**
 * CloudMouse SDK - Preferences Manager Implementation
 *
 * Implementation of persistent storage management using ESP32 NVS (Non-Volatile Storage).
 * Handles device configuration, WiFi credentials with safe access patterns.
 */

#include "PreferencesManager.h"

namespace CloudMouse::Prefs
{
    // ============================================================================
    // SYSTEM INITIALIZATION
    // ============================================================================

    void PreferencesManager::init()
    {
        initDeviceSettings();
    }

    void PreferencesManager::begin(bool readOnly)
    {
        preferences.begin(space, readOnly);
    }

    void PreferencesManager::end()
    {
        preferences.end();
    }

    // ============================================================================
    // WIFI CREDENTIAL MANAGEMENT
    // ============================================================================

    bool PreferencesManager::hasWiFiCredentials()
    {
        return !(getWiFiSSID().equals("") || getWiFiPassword().equals(""));
    }

    void PreferencesManager::saveWiFiCredentials(const String &ssid, const String &password)
    {
        begin(false);
        preferences.putString("wifi_ssid", ssid);
        preferences.putString("wifi_password", password);
        end();
    }

    String PreferencesManager::getWiFiSSID()
    {
        begin(true);
        String value = preferences.getString("wifi_ssid", "");
        end();
        return value;
    }

    String PreferencesManager::getWiFiPassword()
    {
        begin(true);
        String value = preferences.getString("wifi_password", "");
        end();
        return value;
    }

    // ============================================================================
    // GENERIC STORAGE INTERFACE
    // ============================================================================

    void PreferencesManager::save(const char *key, const String &value)
    {
        begin(false);
        preferences.putString(key, value);
        end();
    }

    String PreferencesManager::get(const char *key)
    {
        begin(true);
        String value = preferences.getString(key, "");
        end();
        return value;
    }

    // ============================================================================
    // RESET OPERATIONS
    // ============================================================================

    void PreferencesManager::clear()
    {
        begin(false);
        preferences.clear(); // Clear all preferences in current namespace
        end();

        // Reinitialize namespace to ensure clean state
        preferences.begin("my-app", false);
        preferences.clear();
        end();
    }

    void PreferencesManager::clearAll()
    {
        Serial.println("🗑️ Clearing all preferences...");
        begin(false);
        preferences.clear();
        end();
        Serial.println("✅ All preferences cleared!");
    }

    // ============================================================================
    // DEVICE SETTINGS INITIALIZATION
    // ============================================================================

    void PreferencesManager::initDeviceSettings()
    {
        // Initialize brightness setting if not present
        if (get("conf.brightness").equals(""))
        {
            save("conf.brightness", String(DEFAULT_BRIGHTNESS));
        }

        // Initialize LED color setting if not present
        if (get("conf.ledColor").equals(""))
        {
            save("conf.ledColor", DEFAULT_LED_COLOR);
        }

        // Initialize language setting if not present
        if (get("conf.language").equals(""))
        {
            save("conf.language", DEFAULT_LANGUAGE);
        }

        // Initialize theme setting if not present
        if (get("conf.theme").equals(""))
        {
            save("conf.theme", DEFAULT_THEME);
        }
    }

} // namespace CloudMouse::Prefs