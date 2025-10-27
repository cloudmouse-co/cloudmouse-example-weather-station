/**
 * CloudMouse SDK - NTP Time Manager Implementation
 *
 * Network Time Protocol client implementation with configurable timezone support.
 * Provides reliable time synchronization using multiple NTP server fallbacks.
 */

#include "./NTPManager.h"

const long ITALIAN_GMT_OFFSET_SEC = 3600;
const int ITALIAN_DST_OFFSET_SEC = 0;

namespace CloudMouse::Utils
{

    // Static member initialization
    bool NTPManager::timeInitialized = false;
    long NTPManager::gmtOffset_sec = 0;     // Default to UTC
    int NTPManager::daylightOffset_sec = 0; // Default no DST

    // Default NTP servers (reliable global servers)
    const char *NTPManager::DEFAULT_NTP_SERVER1 = "pool.ntp.org";
    const char *NTPManager::DEFAULT_NTP_SERVER2 = "time.nist.gov";
    const char *NTPManager::DEFAULT_NTP_SERVER3 = "time.google.com";

    // Current NTP servers (configurable)
    const char *NTPManager::ntpServer1 = DEFAULT_NTP_SERVER1;
    const char *NTPManager::ntpServer2 = DEFAULT_NTP_SERVER2;
    const char *NTPManager::ntpServer3 = DEFAULT_NTP_SERVER3;

    // ============================================================================
    // INITIALIZATION
    // ============================================================================

    void NTPManager::init()
    {
        // Initialize with Europe/Rome timezone (GMT+1/GMT+2)
        init(ITALIAN_GMT_OFFSET_SEC, ITALIAN_DST_OFFSET_SEC);
    }

    void NTPManager::init(long gmtOffsetSec, int dstOffsetSec)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("⏰ WiFi not connected - cannot initialize NTP");
            return;
        }

        Serial.println("⏰ Initializing NTP time synchronization...");

        // Store timezone configuration
        gmtOffset_sec = gmtOffsetSec;
        daylightOffset_sec = dstOffsetSec;

        // Configure time with multiple NTP servers for reliability
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);

        // Wait for time synchronization (max 10 seconds)
        int timeout = 0;
        while (!isTimeSet() && timeout < 100)
        {
            delay(100);
            timeout++;
        }

        if (isTimeSet())
        {
            timeInitialized = true;
            Serial.println("✅ NTP synchronized successfully");
            printCurrentTime();
        }
        else
        {
            Serial.println("❌ NTP synchronization failed - check network connection");
        }
    }

    // ============================================================================
    // STATUS FUNCTIONS
    // ============================================================================

    bool NTPManager::isTimeSet()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            return false;
        }
        // Check if year is reasonable (2024+)
        return (timeinfo.tm_year + 1900) >= 2024;
    }

    bool NTPManager::isInitialized()
    {
        return timeInitialized;
    }

    // ============================================================================
    // LOCAL TIME FUNCTIONS
    // ============================================================================

    String NTPManager::getCurrentDateTime()
    {
        if (!timeInitialized || !isTimeSet())
        {
            Serial.println("⚠️ NTP not initialized or time not synchronized");
            return "1970-01-01 00:00:00"; // Unix epoch as fallback
        }

        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("❌ Failed to obtain local time");
            return "1970-01-01 00:00:00";
        }

        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return String(buffer);
    }

    String NTPManager::getCurrentDate()
    {
        if (!timeInitialized || !isTimeSet())
        {
            Serial.println("⚠️ NTP not initialized or time not synchronized");
            return "1970-01-01";
        }

        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("❌ Failed to obtain local time");
            return "1970-01-01";
        }

        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
        return String(buffer);
    }

    String NTPManager::getCurrentTime()
    {
        if (!timeInitialized || !isTimeSet())
        {
            return "00:00:00";
        }

        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            return "00:00:00";
        }

        char buffer[9];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return String(buffer);
    }

    void NTPManager::printCurrentTime()
    {
        if (!isTimeSet())
        {
            Serial.println("⏰ Time not available");
            return;
        }

        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            Serial.printf("⏰ Current local time: %s\n", getCurrentDateTime().c_str());
            Serial.printf("⏰ Current date: %s\n", getCurrentDate().c_str());

            // Show timezone info
            int offsetHours = gmtOffset_sec / 3600;
            int offsetMinutes = (abs(gmtOffset_sec) % 3600) / 60;
            Serial.printf("⏰ Timezone: UTC%+d:%02d\n", offsetHours, offsetMinutes);
        }
    }

    // ============================================================================
    // UTC TIME FUNCTIONS
    // ============================================================================

    String NTPManager::getCurrentDateTimeUTC()
    {
        if (!timeInitialized || !isTimeSet())
        {
            Serial.println("⚠️ NTP not initialized or time not synchronized");
            return "1970-01-01T00:00:00Z";
        }

        // Get UTC timestamp
        time_t now = time(nullptr);
        struct tm *utcTime = gmtime(&now);

        if (!utcTime)
        {
            Serial.println("❌ Failed to get UTC time");
            return "1970-01-01T00:00:00Z";
        }

        char buffer[25];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", utcTime);
        return String(buffer);
    }

    String NTPManager::getCurrentDateUTC()
    {
        if (!timeInitialized || !isTimeSet())
        {
            return "1970-01-01";
        }

        time_t now = time(nullptr);
        struct tm *utcTime = gmtime(&now);

        if (!utcTime)
        {
            return "1970-01-01";
        }

        char buffer[12];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", utcTime);
        return String(buffer);
    }

    String NTPManager::getCurrentTimeUTC()
    {
        if (!timeInitialized || !isTimeSet())
        {
            return "00:00:00";
        }

        time_t now = time(nullptr);
        struct tm *utcTime = gmtime(&now);

        if (!utcTime)
        {
            return "00:00:00";
        }

        char buffer[10];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", utcTime);
        return String(buffer);
    }

    void NTPManager::printCurrentTimeUTC()
    {
        if (!isTimeSet())
        {
            Serial.println("⏰ Time not available");
            return;
        }

        time_t now = time(nullptr);
        struct tm *utcTime = gmtime(&now);
        struct tm localTimeInfo;
        getLocalTime(&localTimeInfo);

        if (utcTime)
        {
            Serial.printf("🌍 UTC time: %s\n", getCurrentDateTimeUTC().c_str());
            Serial.printf("📍 Local time: %s\n", getCurrentDateTime().c_str());

            // Show timezone offset
            int offsetHours = gmtOffset_sec / 3600;
            int offsetMinutes = (abs(gmtOffset_sec) % 3600) / 60;
            Serial.printf("⏰ Timezone offset: UTC%+d:%02d\n", offsetHours, offsetMinutes);
        }
    }

    // ============================================================================
    // TIMESTAMP FUNCTIONS
    // ============================================================================

    time_t NTPManager::getEpochTime()
    {
        if (!timeInitialized || !isTimeSet())
        {
            Serial.println("⚠️ NTP not initialized or time not synchronized");
            return 0;
        }
        return time(nullptr); // Unix timestamp in seconds
    }

    // ============================================================================
    // CONFIGURATION FUNCTIONS
    // ============================================================================

    void NTPManager::setTimezone(long gmtOffsetSec, int dstOffsetSec)
    {
        gmtOffset_sec = gmtOffsetSec;
        daylightOffset_sec = dstOffsetSec;

        // Reconfigure time if already initialized
        if (timeInitialized && WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("⏰ Updating timezone to UTC%+d:%02d\n",
                          (int)(gmtOffsetSec / 3600),
                          (int)((abs(gmtOffsetSec) % 3600) / 60));
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
        }
    }

    void NTPManager::setNTPServers(const char *server1, const char *server2, const char *server3)
    {
        ntpServer1 = server1 ? server1 : DEFAULT_NTP_SERVER1;
        ntpServer2 = server2 ? server2 : DEFAULT_NTP_SERVER2;
        ntpServer3 = server3 ? server3 : DEFAULT_NTP_SERVER3;

        Serial.printf("⏰ NTP servers updated: %s, %s, %s\n", ntpServer1, ntpServer2, ntpServer3);

        // Reinitialize if already configured
        if (timeInitialized && WiFi.status() == WL_CONNECTED)
        {
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
        }
    }

} // namespace CloudMouse