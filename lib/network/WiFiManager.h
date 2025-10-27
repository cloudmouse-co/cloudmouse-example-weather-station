/**
 * CloudMouse SDK - WiFi Connection Manager
 *
 * Comprehensive WiFi lifecycle management with multiple connection methods and fallback strategies.
 * Handles automatic reconnection, credential persistence, and device setup modes for IoT applications.
 *
 * Features:
 * - Automatic connection with saved credentials from NVS storage
 * - Manual connection with timeout handling and retry logic
 * - Access Point mode for device setup and configuration
 * - WPS (WiFi Protected Setup) push-button configuration
 * - Event-driven state management with comprehensive status reporting
 * - Automatic NTP time synchronization on successful connection
 * - Integration with PreferencesManager for credential persistence
 * - Device-specific AP credentials using hardware MAC address
 *
 * Connection Flow:
 * 1. Initialize and attempt saved credentials
 * 2. If no credentials or connection fails, enter AP mode
 * 3. User configures via web interface or WPS
 * 4. Automatic reconnection on subsequent boots
 * 5. NTP sync and operational mode activation
 *
 * State Machine:
 * DISCONNECTED → CONNECTING → CONNECTED (success path)
 *            ↘ TIMEOUT → AP_MODE_INIT → AP_MODE (setup path)
 *            ↘ WPS_LISTENING → WPS_SUCCESS/WPS_FAILED (WPS path)
 *
 * Usage:
 * 1. Create instance and call init() during system startup
 * 2. Call update() regularly in main loop for timeout handling
 * 3. Monitor state changes via getState() or status query methods
 * 4. Integrate with WebServerManager for captive portal setup
 */

#pragma once
#include <WiFi.h>
#include <esp_wps.h>
#include <string>
#include "../prefs/PreferencesManager.h"
#include "../utils/DeviceID.h"
#include "../config/DeviceConfig.h"

using namespace CloudMouse::Utils;
using namespace CloudMouse::Prefs;
namespace CloudMouse::Network
{
    class WiFiManager
    {
    public:
        /**
         * WiFi connection state enumeration
         * Represents all possible states in the WiFi lifecycle
         */
        enum class WiFiState
        {
            DISCONNECTED,         // No WiFi connection, idle state
            CONNECTING,           // Attempting connection to network
            CONNECTED,            // Successfully connected with IP address
            TIMEOUT,              // Connection attempt timed out
            AP_MODE_INIT,         // Initializing Access Point mode
            AP_MODE,              // Access Point active, awaiting configuration
            WPS_LISTENING,        // WPS mode active, waiting for button press
            WPS_SUCCESS,          // WPS configuration received successfully
            WPS_FAILED,           // WPS configuration failed or timed out
            ERROR,                // General error state
            CREDENTIAL_NOT_FOUND, // No saved credentials available
        };

        /**
         * Constructor - prepares WiFi manager instance
         * Sets up static instance pointer for event callback system
         */
        WiFiManager();
        ~WiFiManager() = default;

        /**
         * Initialize WiFi manager and begin connection process
         * Registers event handlers and attempts automatic connection
         * Call once during system initialization
         */
        void init();

        /**
         * Update WiFi manager state and handle timeouts
         * Processes connection timeouts and state transitions
         * Must be called regularly in main loop (every 100ms recommended)
         */
        void update();

        // ========================================================================
        // CONNECTION MANAGEMENT
        // ========================================================================

        /**
         * Attempt connection using saved credentials from NVS
         * Automatically retrieves SSID and password from preferences
         *
         * @return true if credentials found and connection attempt started
         */
        bool connectWithSavedCredentials();

        /**
         * Connect to specified WiFi network with timeout
         * Saves successful credentials automatically for future use
         *
         * @param ssid Network SSID (case-sensitive)
         * @param password Network password (WPA/WPA2/WPA3)
         * @param timeout Connection timeout in milliseconds (default: 10 seconds)
         * @return true if connection attempt started successfully
         */
        bool connect(const char *ssid, const char *password, uint32_t timeout = 10000);

        /**
         * Disconnect from current WiFi network
         * Transitions to DISCONNECTED state immediately
         */
        void disconnect();

        /**
         * Attempt reconnection using saved credentials
         * Convenience method for retry logic after failures
         */
        void reconnect();

        // ========================================================================
        // ACCESS POINT MODE
        // ========================================================================

        /**
         * Setup device as WiFi Access Point for configuration
         * Creates network with device-specific SSID and password
         * IP address: 192.168.4.1 (standard AP gateway)
         * Use with WebServerManager for captive portal setup
         */
        void setupAP();

        /**
         * Stop Access Point mode and return to station mode
         * Disconnects all connected clients gracefully
         */
        void stopAP();

        /**
         * Check if devices are connected to our Access Point
         * Useful for determining if setup is in progress
         *
         * @return true if one or more clients connected to AP
         */
        bool hasConnectedDevices();

        /**
         * Get Access Point SSID (device-specific)
         * Format: "CloudMouse-{device_id}" based on MAC address
         *
         * @return AP network name for user connection
         */
        String getAPSSID() const { return DeviceID::getAPSSID(); }

        /**
         * Get Access Point password (device-specific)
         * Generated from device MAC address for security
         *
         * @return AP password for user authentication
         */
        String getAPPassword() const { return DeviceID::getAPPassword(); }

        /**
         * Get Access Point IP address
         * Standard gateway address for device configuration
         *
         * @return AP IP address string (typically "192.168.4.1")
         */
        String getAPIP() const { return "192.168.4.1"; }

        // ========================================================================
        // WPS (WiFi Protected Setup) SUPPORT
        // ========================================================================

        /**
         * Start WPS push-button configuration mode
         * Listens for WPS button press on router for 2 minutes
         * Alternative to manual credential entry
         */
        void startWPS();

        /**
         * Stop WPS mode and return to normal operation
         * Call after successful WPS connection or timeout
         */
        void stopWPS();

        /**
         * Check if WPS mode is currently active
         *
         * @return true if listening for WPS button press
         */
        bool isWPSListening() const { return wpsStarted; }

        // ========================================================================
        // STATUS QUERIES
        // ========================================================================

        /**
         * Get current WiFi connection state
         *
         * @return Current state from WiFiState enumeration
         */
        WiFiState getState() const { return currentState; }

        /**
         * Check if connected to WiFi network with IP address
         *
         * @return true if fully connected and operational
         */
        bool isConnected() const { return currentState == WiFiState::CONNECTED; }

        /**
         * Check if connection attempt is in progress
         *
         * @return true if currently attempting to connect
         */
        bool isConnecting() const { return currentState == WiFiState::CONNECTING; }

        /**
         * Check if last connection attempt timed out
         *
         * @return true if connection timeout occurred
         */
        bool isTimedOut() const { return currentState == WiFiState::TIMEOUT; }

        /**
         * Check if device is in Access Point mode
         *
         * @return true if AP mode is active
         */
        bool isAPMode() const { return currentState == WiFiState::AP_MODE; }

        /**
         * Get local IP address (station or AP mode)
         *
         * @return IP address string, empty if not connected
         */
        String getLocalIP() const;

        /**
         * Get connected network SSID or AP SSID
         *
         * @return Network name, empty if not connected
         */
        String getSSID() const;

        /**
         * Get WiFi signal strength (RSSI)
         * Only valid when connected to a network
         *
         * @return Signal strength in dBm (negative values, closer to 0 = stronger)
         */
        int getRSSI() const;

        /**
         * Get current connection attempt duration
         * Returns elapsed time since connection started
         *
         * @return Connection time in milliseconds, 0 if not connecting
         */
        uint32_t getConnectionTime() const;

        /**
         * Get current WiFi mode (station, AP, or mixed)
         *
         * @return ESP32 WiFi mode enumeration
         */
        wifi_mode_t getMode() const { return WiFi.getMode(); }

        // ========================================================================
        // CONFIGURATION MANAGEMENT
        // ========================================================================

        /**
         * Save WiFi credentials to persistent storage
         * Credentials are encrypted and stored in NVS for automatic reconnection
         *
         * @param ssid Network SSID to save
         * @param password Network password to save
         */
        void saveCredentials(const String &ssid, const String &password);

        /**
         * Check if clients are connected to Access Point
         * Alias for hasConnectedDevices() for API consistency
         *
         * @return true if AP has connected clients
         */
        bool hasAPClient() const { return WiFi.softAPgetStationNum() > 0; }

    private:
        // Current state and configuration
        WiFiState currentState = WiFiState::DISCONNECTED;
        PreferencesManager prefs; // Credential storage

        // Connection timing and timeout handling
        uint32_t connectionStartTime = 0;   // Connection attempt start time
        uint32_t connectionTimeout = 10000; // Default timeout (10 seconds)

        // Feature flags and initialization status
        bool wpsStarted = false;  // WPS mode active flag
        bool initialized = false; // Manager initialization status

        // Static instance pointer for ESP32 event callback system
        static WiFiManager *staticInstance;

        /**
         * Update internal state and trigger state change logging
         *
         * @param newState Target state to transition to
         */
        void setState(WiFiState newState);

        /**
         * Check for connection timeout and handle failure
         * Called from update() when in CONNECTING state
         */
        void handleConnectionTimeout();

        /**
         * Static callback for ESP32 WiFi events
         * Handles connection success, failure, and WPS events
         *
         * @param event WiFi event type from ESP32 system
         * @param info Event-specific information structure
         */
        static void WiFiEventHandler(WiFiEvent_t event, arduino_event_info_t info);
    };
};