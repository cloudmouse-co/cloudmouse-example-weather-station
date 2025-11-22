/**
 * @file PreferencesManager.h
 * @brief Persistent storage manager for ESP32 NVS
 *
 * Provides thread-safe access to device configuration, WiFi credentials,
 * and generic key-value storage with automatic initialization and default values.
 *
 * @namespace CloudMouse::Prefs
 * @brief Preferences management components
 */

#ifndef PREFERENCESMANAGER_H
#define PREFERENCESMANAGER_H

#include <Preferences.h>

namespace CloudMouse::Prefs
{
    /**
     * @class PreferencesManager
     * @brief Manages persistent storage using ESP32 NVS
     *
     * Handles device configuration, WiFi credentials, and generic key-value storage
     * with thread-safe operations using FreeRTOS mutexes.
     *
     * Features:
     * - Automatic initialization with default device settings
     * - Thread-safe WiFi credential management
     * - Batch operations for improved performance
     * - Generic key-value storage interface
     * - Safe clear/reset operations
     *
     * @note All operations are thread-safe and protected by mutex
     */
    class PreferencesManager
    {
    public:
        /**
         * @brief Initialize the preferences manager
         *
         * Creates mutex for thread-safe access and sets default values
         * for device settings if not already present.
         */
        void init();

        /**
         * @brief Check if WiFi credentials are stored
         * @return True if both SSID and password are present
         */
        bool hasWiFiCredentials();

        /**
         * @brief Save WiFi credentials to NVS
         * @param ssid WiFi network SSID
         * @param password WiFi network password
         */
        void saveWiFiCredentials(const String &ssid, const String &password);

        /**
         * @brief Retrieve stored WiFi SSID
         * @return WiFi SSID or empty string if not set
         */
        String getWiFiSSID();

        /**
         * @brief Retrieve stored WiFi password
         * @return WiFi password or empty string if not set
         */
        String getWiFiPassword();

        /**
         * @brief Begin batch operation for multiple read/write operations
         * @param readOnly If true, opens in read-only mode (default: false)
         * @return True if batch was successfully opened
         *
         * Keeps NVS namespace open for multiple operations to improve performance.
         * Supports nested calls with depth tracking. Automatically retries on failure.
         *
         * @note Must be followed by endBatch() to release resources
         */
        bool beginBatch(bool readOnly = false);

        /**
         * @brief End batch operation and close NVS namespace
         *
         * Handles nested calls by tracking depth. Only closes namespace
         * when depth reaches zero.
         */
        void endBatch();

        /**
         * @brief Check if batch operation is currently open
         * @return True if batch is open
         */
        bool isBatchOpen() const { return batchOpen; }

        /**
         * @brief Save string during batch operation
         * @param key Storage key
         * @param value String value to store
         * @return True if save was successful
         *
         * If batch is not open, falls back to normal save().
         */
        bool putString(const char *key, const String &value);

        /**
         * @brief Retrieve string during batch operation
         * @param key Storage key
         * @param defaultValue Value to return if key not found (default: empty string)
         * @return Stored value or defaultValue if not found
         *
         * If batch is not open, falls back to normal get().
         */
        String getString(const char *key, const String &defaultValue = "");

        /**
         * @brief Save a string value to NVS
         * @param key Storage key
         * @param value String value to store
         * @return True if save was successful
         */
        bool save(const char *key, const String &value);

        /**
         * @brief Retrieve a string value from NVS
         * @param key Storage key
         * @return Stored value or empty string if not found
         */
        String get(const char *key);

        /**
         * @brief Clear all preferences in current namespace
         *
         * Removes all stored key-value pairs and reinitializes namespace
         * to ensure clean state.
         */
        void clear();

        /**
         * @brief Clear all stored preferences
         *
         * Complete wipe of all preferences data across all namespaces.
         */
        void clearAll();

    private:
        /** @brief ESP32 NVS interface */
        Preferences preferences;

        /** @brief Batch operation state flag */
        bool batchOpen = false;

        /** @brief NVS namespace identifier */
        const char *space = "my-app";

        /** @brief Nested batch operation depth counter */
        int batchDepth = 0;

        /** @brief FreeRTOS mutex for thread-safe NVS access */
        SemaphoreHandle_t nvsMutex = NULL;

        /** @brief Default brightness level (0-100) */
        const int DEFAULT_BRIGHTNESS = 80;

        /** @brief Default LED color */
        const String DEFAULT_LED_COLOR = "azure";

        /** @brief Default language code */
        const String DEFAULT_LANGUAGE = "it";

        /** @brief Default theme name */
        const String DEFAULT_THEME = "light";

        /**
         * @brief Open preferences namespace with optional read-only mode
         * @param readOnly If true, opens in read-only mode
         *
         * Thread-safe operation using mutex locking.
         */
        void begin(bool readOnly);

        /**
         * @brief Close preferences namespace and release mutex
         */
        void end();

        /**
         * @brief Initialize device settings with default values
         *
         * Sets default values for brightness, LED color, language, and theme
         * if not already present in storage.
         */
        void initDeviceSettings();
    };
};
#endif