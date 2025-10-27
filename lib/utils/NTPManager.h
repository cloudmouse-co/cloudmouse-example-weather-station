/**
 * CloudMouse SDK - NTP Time Manager
 * 
 * Network Time Protocol client for accurate time synchronization.
 * Provides both local time and UTC time utilities with configurable timezone support.
 * 
 * Features:
 * - Multiple NTP server fallback for reliability
 * - Configurable timezone and DST settings
 * - Local time and UTC time functions
 * - Time validation and synchronization status
 * - Epoch timestamp support for logging
 */

#pragma once
#include <WiFi.h>
#include <time.h>
#include <Arduino.h>

namespace CloudMouse::Utils {

class NTPManager {
public:
    // System lifecycle
    static void init();                         // Initialize NTP with default timezone (UTC)
    static void init(long gmtOffsetSec, int dstOffsetSec = 0); // Initialize with custom timezone
    
    // Time status
    static bool isTimeSet();                    // Check if NTP time is synchronized
    static bool isInitialized();               // Check if NTP manager is initialized
    
    // Local time functions
    static String getCurrentDateTime();         // Get formatted local date and time
    static String getCurrentDate();            // Get formatted local date (YYYY-MM-DD)
    static String getCurrentTime();            // Get formatted local time (HH:MM:SS)
    static void printCurrentTime();            // Print current local time to Serial
    
    // UTC time functions
    static String getCurrentDateTimeUTC();     // Get formatted UTC date and time
    static String getCurrentDateUTC();         // Get formatted UTC date
    static String getCurrentTimeUTC();         // Get formatted UTC time
    static void printCurrentTimeUTC();         // Print UTC and local time comparison
    
    // Timestamp functions
    static time_t getEpochTime();              // Get Unix timestamp (seconds since 1970)
    
    // Configuration
    static void setTimezone(long gmtOffsetSec, int dstOffsetSec = 0); // Update timezone settings
    static void setNTPServers(const char* server1, const char* server2 = nullptr, const char* server3 = nullptr);

private:
    static bool timeInitialized;
    static long gmtOffset_sec;                 // GMT offset in seconds
    static int daylightOffset_sec;             // Daylight saving time offset in seconds
    
    // Default NTP servers (configurable)
    static const char* ntpServer1;
    static const char* ntpServer2;  
    static const char* ntpServer3;
    
    // Default server addresses
    static const char* DEFAULT_NTP_SERVER1;
    static const char* DEFAULT_NTP_SERVER2;
    static const char* DEFAULT_NTP_SERVER3;
};

}  // namespace CloudMouse