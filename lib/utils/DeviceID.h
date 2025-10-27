/**
 * CloudMouse SDK - Device Identity Manager
 *
 * Provides device identification utilities using ESP32 hardware features.
 * Generates unique IDs, UUIDs, and Access Point credentials based on device MAC address.
 *
 * Features:
 * - Deterministic device ID generation
 * - UUID creation (hardware-based)
 * - Access Point SSID/password generation
 * - Device information logging
 */

#ifndef DEVICEID_H
#define DEVICEID_H

#include <Arduino.h>
#include <esp_system.h>

namespace CloudMouse::Utils
{
    class DeviceID
    {
    public:
        // Get unique ESP32 device ID (last 4 bytes of MAC address)
        static String getDeviceID()
        {
            uint64_t chipid = ESP.getEfuseMac();
            uint32_t low = (uint32_t)chipid;

            char id[9];
            snprintf(id, sizeof(id), "%08x", low);

            return String(id);
        }

        // Generate hardware-based UUID using complete MAC address
        static String getDeviceUUID()
        {
            uint64_t mac = ESP.getEfuseMac();
            uint8_t *macBytes = (uint8_t *)&mac;

            // UUID format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
            // Uses MAC (6 bytes = 48 bits) + chip ID to fill 128 bits

            uint32_t chipID = (uint32_t)ESP.getEfuseMac();
            uint16_t chipRev = ESP.getChipRevision();

            char uuid[37];
            snprintf(uuid, sizeof(uuid),
                     "%02x%02x%02x%02x-%02x%02x-4%01x%02x-%02x%02x-%02x%02x%02x%02x%04x",
                     macBytes[5], macBytes[4], macBytes[3], macBytes[2], // 8 chars
                     macBytes[1], macBytes[0],                           // 4 chars
                     (chipID >> 12) & 0x0F, (chipID >> 8) & 0xFF,        // 4 chars (version 4)
                     0x80 | ((chipID >> 4) & 0x3F), chipID & 0xFF,       // 4 chars (variant)
                     macBytes[5], macBytes[4], macBytes[3],              // 6 chars
                     macBytes[2], macBytes[1], chipRev                   // 6 chars
            );

            return String(uuid);
        }

        // Alternative UUID v4 style using MAC as deterministic seed
        static String getDeviceUUIDv4Style()
        {
            uint64_t mac = ESP.getEfuseMac();

            // Use MAC as seed for deterministic pseudo-random numbers
            uint32_t part1 = (uint32_t)(mac & 0xFFFFFFFF);
            uint32_t part2 = (uint32_t)((mac >> 32) & 0xFFFF);
            uint32_t part3 = ESP.getChipRevision() * 0x1A2B3C4D;
            uint32_t part4 = part1 ^ part2;

            char uuid[37];
            snprintf(uuid, sizeof(uuid),
                     "%08x-%04x-4%03x-%04x-%08x%04x",
                     part1,                                 // 8 characters
                     (uint16_t)(part2 & 0xFFFF),            // 4 characters
                     (uint16_t)(part3 & 0xFFF),             // 3 characters (+ "4" for version)
                     (uint16_t)(0x8000 | (part4 & 0x3FFF)), // 4 characters (variant)
                     part1 ^ part3,                         // 8 characters
                     (uint16_t)(part2 ^ part4)              // 4 characters
            );

            return String(uuid);
        }

        // Generate Access Point SSID
        static String getAPSSID()
        {
            return "CloudMouse-" + getDeviceID();
        }

        // Generate simple AP password (first 8 characters of device ID)
        static String getAPPassword()
        {
            String id = getDeviceID();
            return id.substring(0, 8);
        }

        // Generate secure AP password with MAC byte mixing
        static String getAPPasswordSecure()
        {
            uint64_t mac = ESP.getEfuseMac();
            uint8_t *macBytes = (uint8_t *)&mac;

            // Create password by mixing MAC address bytes
            char pass[11];
            snprintf(pass, sizeof(pass), "%02x%02x%02x%02x%02x",
                     macBytes[0] ^ macBytes[3],
                     macBytes[1] ^ macBytes[4],
                     macBytes[2] ^ macBytes[5],
                     macBytes[3] ^ macBytes[0],
                     macBytes[4] ^ macBytes[1]);

            return String(pass);
        }

        // Get formatted MAC address
        static String getMACAddress()
        {
            uint64_t mac = ESP.getEfuseMac();
            uint8_t *macBytes = (uint8_t *)&mac;

            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     macBytes[0], macBytes[1], macBytes[2],
                     macBytes[3], macBytes[4], macBytes[5]);

            return String(macStr);
        }

        // Print comprehensive device information
        static void printDeviceInfo()
        {
            Serial.println("\n📱 Device Information:");
            Serial.printf("   Device ID: %s\n", getDeviceID().c_str());
            Serial.printf("   Device UUID: %s\n", getDeviceUUID().c_str());
            Serial.printf("   MAC Address: %s\n", getMACAddress().c_str());
            Serial.printf("   AP SSID: %s\n", getAPSSID().c_str());
            Serial.printf("   AP Password: %s\n", getAPPassword().c_str());
            Serial.printf("   AP Password (Secure): %s\n", getAPPasswordSecure().c_str());
            Serial.printf("   Chip Model: %s\n", ESP.getChipModel());
            Serial.printf("   Chip Revision: %d\n", ESP.getChipRevision());
            Serial.printf("   CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
            Serial.println();
        }
    };
};
#endif