/**
 * CloudMouse SDK - JSON Helper Utilities
 *
 * Provides JSON parsing and handling utilities for HTTP responses and configuration.
 * Uses ArduinoJson library with PSRAM support for memory-efficient operations.
 *
 * Features:
 * - Safe JSON deserialization with error handling
 * - Dynamic memory allocation with PSRAM support
 * - HTTP response parsing utilities
 */

#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <ArduinoJson.h>
#include <esp_heap_caps.h>

namespace CloudMouse::Utils
{
    class JsonHelper
    {
    public:
        /**
         * Parse JSON string into DynamicJsonDocument
         *
         * @param payload JSON string to parse
         * @return DynamicJsonDocument containing parsed data, or empty document on error
         */
        static DynamicJsonDocument parseJson(const String &payload)
        {
            // Calculate required capacity (object size + string data + safety margin)
            const size_t capacity = JSON_OBJECT_SIZE(20) + payload.length() + 1000;

            // Create document with dynamic memory allocation (uses PSRAM if available)
            DynamicJsonDocument doc(capacity);

            // Parse JSON payload
            DeserializationError error = deserializeJson(doc, payload);

            // Handle parsing errors
            if (error)
            {
                SDK_LOGGER("❌ JSON parsing error: %s\n", error.c_str());
                return DynamicJsonDocument(0); // Return empty document on error
            }

            return doc;
        }

        /**
         * Parse HTTP response JSON payload
         * Legacy method name for backward compatibility
         *
         * @param payload HTTP response body containing JSON
         * @return DynamicJsonDocument containing parsed data
         */
        static DynamicJsonDocument decodeResponse(const String &payload)
        {
            return parseJson(payload);
        }

        /**
         * Check if JSON document is valid (not empty)
         *
         * @param doc JSON document to validate
         * @return true if document contains valid data
         */
        static bool isValidJson(const DynamicJsonDocument &doc)
        {
            return !doc.isNull() && doc.capacity() > 0;
        }

        /**
         * Get string value from JSON with fallback
         *
         * @param doc JSON document
         * @param key Key to lookup
         * @param defaultValue Default value if key not found
         * @return String value or default
         */
        static String getString(const DynamicJsonDocument &doc, const char *key, const String &defaultValue = "")
        {
            if (doc.containsKey(key))
            {
                return doc[key].as<String>();
            }
            return defaultValue;
        }

        /**
         * Get integer value from JSON with fallback
         *
         * @param doc JSON document
         * @param key Key to lookup
         * @param defaultValue Default value if key not found
         * @return Integer value or default
         */
        static int getInt(const DynamicJsonDocument &doc, const char *key, int defaultValue = 0)
        {
            if (doc.containsKey(key))
            {
                return doc[key].as<int>();
            }
            return defaultValue;
        }

        /**
         * Get boolean value from JSON with fallback
         *
         * @param doc JSON document
         * @param key Key to lookup
         * @param defaultValue Default value if key not found
         * @return Boolean value or default
         */
        static bool getBool(const DynamicJsonDocument &doc, const char *key, bool defaultValue = false)
        {
            if (doc.containsKey(key))
            {
                return doc[key].as<bool>();
            }
            return defaultValue;
        }
    };
};
#endif