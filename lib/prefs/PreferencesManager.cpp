/**
 * @file PreferencesManager.cpp
 * @brief Implementation of persistent storage manager using ESP32 NVS
 * 
 * Provides thread-safe access to device configuration, WiFi credentials,
 * and generic key-value storage with automatic initialization.
 */

#include "PreferencesManager.h"
#include "../utils/Logger.h"

namespace CloudMouse::Prefs
{
    // ============================================================================
    // SYSTEM INITIALIZATION
    // ============================================================================

    /**
     * @brief Initialize the preferences manager
     * 
     * Creates mutex for thread-safe NVS access and initializes device settings
     * with default values if not already present.
     */
    void PreferencesManager::init()
    {
        // Create mutex for thread-safe access
        if (nvsMutex == NULL)
        {
            nvsMutex = xSemaphoreCreateMutex();
            if (nvsMutex == NULL)
            {
                SDK_LOGGER("❌ FATAL: Failed to create NVS mutex!");
            }
            else
            {
                SDK_LOGGER("✅ NVS mutex created");
            }
        }

        initDeviceSettings();
    }

    /**
     * @brief Open preferences namespace with optional read-only mode
     * @param readOnly If true, opens in read-only mode
     * 
     * Thread-safe operation using mutex locking.
     */
    void PreferencesManager::begin(bool readOnly)
    {
        if (nvsMutex != NULL)
        {
            xSemaphoreTake(nvsMutex, portMAX_DELAY);
        }

        preferences.begin(space, readOnly);
    }

    /**
     * @brief Close preferences namespace and release mutex
     */
    void PreferencesManager::end()
    {
        preferences.end();

        if (nvsMutex != NULL)
        {
            xSemaphoreGive(nvsMutex);
        }
    }

    // ============================================================================
    // WIFI CREDENTIAL MANAGEMENT
    // ============================================================================

    /**
     * @brief Check if WiFi credentials are stored
     * @return True if both SSID and password are present
     */
    bool PreferencesManager::hasWiFiCredentials()
    {
        return !(getWiFiSSID().equals("") || getWiFiPassword().equals(""));
    }

    /**
     * @brief Save WiFi credentials to NVS
     * @param ssid WiFi network SSID
     * @param password WiFi network password
     */
    void PreferencesManager::saveWiFiCredentials(const String &ssid, const String &password)
    {
        begin(false);
        preferences.putString("wifi_ssid", ssid);
        preferences.putString("wifi_password", password);
        end();
    }

    /**
     * @brief Retrieve stored WiFi SSID
     * @return WiFi SSID or empty string if not set
     */
    String PreferencesManager::getWiFiSSID()
    {
        begin(true);
        String value = preferences.getString("wifi_ssid", "");
        end();
        return value;
    }

    /**
     * @brief Retrieve stored WiFi password
     * @return WiFi password or empty string if not set
     */
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

    /**
     * @brief Save a string value to NVS
     * @param key Storage key
     * @param value String value to store
     * @return True if save was successful
     */
    bool PreferencesManager::save(const char *key, const String &value)
    {
        begin(false);
        preferences.putString(key, value);
        end();
        return true;
    }

    /**
     * @brief Retrieve a string value from NVS
     * @param key Storage key
     * @return Stored value or empty string if not found
     */
    String PreferencesManager::get(const char *key)
    {
        begin(true);
        String value = preferences.getString(key, "");
        end();
        return value;
    }

    // ============================================================================
    // BATCH OPERATIONS
    // ============================================================================

    /**
     * @brief Begin batch operation for multiple read/write operations
     * @param readOnly If true, opens in read-only mode
     * @return True if batch was successfully opened
     * 
     * Keeps NVS namespace open for multiple operations to improve performance.
     * Supports nested calls with depth tracking. Automatically retries on failure.
     */
    bool PreferencesManager::beginBatch(bool readOnly)
    {

        // Lock mutex FIRST
        if (nvsMutex != NULL)
        {
            xSemaphoreTake(nvsMutex, portMAX_DELAY);
        }

        // Check if already open (nested call)
        if (batchOpen)
        {
            SDK_LOGGER("⚠️ Batch already open (nested call), keeping it open");
            batchDepth++;
            return true;
        }

        // Try up to 3 times with small delay
        for (int attempt = 0; attempt < 3; attempt++)
        {
            batchOpen = preferences.begin(space, readOnly);

            if (batchOpen)
            {
                batchDepth = 1;
                if (attempt > 0)
                {
                    SDK_LOGGER("✅ Batch opened on attempt %d\n", attempt + 1);
                }
                return true;
            }

            SDK_LOGGER("⚠️ Batch open failed (attempt %d/3), retrying...\n", attempt + 1);
            delay(10);
        }

        SDK_LOGGER("❌ Batch open FAILED after 3 attempts! (namespace=%s)\n", space);
        
        // Release mutex on failure
        if (nvsMutex != NULL)
        {
            xSemaphoreGive(nvsMutex);
        }
        
        return false;
    }

    /**
     * @brief End batch operation and close NVS namespace
     * 
     * Handles nested calls by tracking depth. Only closes namespace when
     * depth reaches zero.
     */
    void PreferencesManager::endBatch()
    {
        if (!batchOpen)
        {
            // Not open, but still release mutex if taken
            if (nvsMutex != NULL && batchDepth > 0)
            {
                xSemaphoreGive(nvsMutex);
                batchDepth = 0;
            }
            return;
        }

        // Handle nested calls
        batchDepth--;
        if (batchDepth > 0)
        {
            SDK_LOGGER("⚠️ Nested batch, depth now: %d\n", batchDepth);
            return;
        }

        // Actually close
        preferences.end();
        batchOpen = false;
        batchDepth = 0;

        // Release mutex LAST
        if (nvsMutex != NULL)
        {
            xSemaphoreGive(nvsMutex);
        }
    }

    /**
     * @brief Save string during batch operation
     * @param key Storage key
     * @param value String value to store
     * @return True if save was successful
     * 
     * If batch is not open, falls back to normal save().
     */
    bool PreferencesManager::putString(const char *key, const String &value)
    {
        if (!batchOpen)
        {
            // Fallback to normal save
            return save(key, value);
        }

        return preferences.putString(key, value.c_str());
    }

    /**
     * @brief Retrieve string during batch operation
     * @param key Storage key
     * @param defaultValue Value to return if key not found
     * @return Stored value or defaultValue if not found
     * 
     * If batch is not open, falls back to normal get().
     */
    String PreferencesManager::getString(const char *key, const String &defaultValue)
    {
        if (!batchOpen)
        {
            // Fallback to normal get
            return get(key);
        }

        return preferences.getString(key, defaultValue.c_str());
    }

    // ============================================================================
    // RESET OPERATIONS
    // ============================================================================

    /**
     * @brief Clear all preferences in current namespace
     * 
     * Removes all stored key-value pairs and reinitializes namespace
     * to ensure clean state.
     */
    void PreferencesManager::clear()
    {
        begin(false);
        preferences.clear(); // Clear all preferences in current namespace
        end();

        // Reinitialize namespace to ensure clean state
        preferences.begin(space, false);
        preferences.clear();
        end();
    }

    /**
     * @brief Clear all stored preferences
     * 
     * Complete wipe of all preferences data.
     */
    void PreferencesManager::clearAll()
    {
        SDK_LOGGER("🗑️ Clearing all preferences...");
        begin(false);
        preferences.clear();
        end();
        SDK_LOGGER("✅ All preferences cleared!");
    }

    // ============================================================================
    // DEVICE SETTINGS INITIALIZATION
    // ============================================================================

    /**
     * @brief Initialize device settings with default values
     * 
     * Sets default values for brightness, LED color, language, and theme
     * if not already present in storage.
     */
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