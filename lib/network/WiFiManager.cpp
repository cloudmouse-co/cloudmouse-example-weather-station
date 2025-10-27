/**
 * CloudMouse SDK - WiFi Connection Manager Implementation
 *
 * Comprehensive WiFi lifecycle management with event-driven architecture and automatic recovery.
 * Integrates NTP time synchronization, credential persistence, and multiple connection fallback methods.
 *
 * Architecture:
 * - Event-driven state machine using ESP32 WiFi events
 * - Automatic credential management via NVS storage
 * - Timeout handling with configurable retry logic
 * - Integration with NTPManager for time synchronization
 * - Device-specific AP configuration using MAC-based credentials
 */

#include "./WiFiManager.h"
#include "../utils/NTPManager.h"

namespace CloudMouse::Network
{
    // Static instance pointer for ESP32 event callback system
    WiFiManager *WiFiManager::staticInstance = nullptr;

    // ============================================================================
    // INITIALIZATION AND LIFECYCLE
    // ============================================================================

    WiFiManager::WiFiManager()
    {
        // Set static instance for event callback system
        // Required because ESP32 WiFi events use C-style callbacks
        staticInstance = this;
    }

    void WiFiManager::init()
    {
        Serial.println("📶 Initializing WiFiManager...");

        // Register WiFi event handler for state management
        // Handles connection success, failure, and WPS events automatically
        WiFi.onEvent(WiFiEventHandler);

        initialized = true;

        // Attempt automatic connection with saved credentials
        if (connectWithSavedCredentials())
        {
            Serial.println("📶 Attempting connection with saved credentials...");
        }
        else
        {
            Serial.println("📶 No saved credentials found - setup required");
            setState(WiFiState::CREDENTIAL_NOT_FOUND);
        }

        Serial.println("✅ WiFiManager initialized successfully");
    }

    void WiFiManager::update()
    {
        if (!initialized)
            return;

        // Handle connection timeout monitoring
        // Only active when in CONNECTING state
        if (currentState == WiFiState::CONNECTING)
        {
            handleConnectionTimeout();
        }
    }

    // ============================================================================
    // CONNECTION MANAGEMENT
    // ============================================================================

    bool WiFiManager::connectWithSavedCredentials()
    {
        // Retrieve credentials from encrypted NVS storage
        String savedSSID = prefs.getWiFiSSID();
        String savedPassword = prefs.getWiFiPassword();

        // Validate credential presence
        if (savedSSID.isEmpty() || savedPassword.isEmpty())
        {
            Serial.println("📶 No valid saved credentials found");
            return false;
        }

        Serial.printf("📶 Found saved credentials for network: %s\n", savedSSID.c_str());
        return connect(savedSSID.c_str(), savedPassword.c_str());
    }

    bool WiFiManager::connect(const char *ssid, const char *password, uint32_t timeout)
    {
        if (!initialized)
        {
            Serial.println("❌ WiFiManager not initialized");
            return false;
        }

        Serial.printf("📶 Initiating connection to WiFi network: %s\n", ssid);

        // Configure WiFi mode and reset any existing connections
        WiFi.mode(WIFI_STA); // Station mode for client connection
        WiFi.disconnect();   // Clear any previous connections
        delay(100);          // Brief stabilization delay

        // Update state and start connection timing
        setState(WiFiState::CONNECTING);
        connectionStartTime = millis();
        connectionTimeout = timeout;

        // Initiate connection attempt
        // Actual connection result handled by WiFiEventHandler callback
        WiFi.begin(ssid, password);

        return true;
    }

    void WiFiManager::disconnect()
    {
        Serial.println("📶 Disconnecting from WiFi network...");
        WiFi.disconnect();
        setState(WiFiState::DISCONNECTED);
    }

    void WiFiManager::reconnect()
    {
        Serial.println("🔄 Attempting WiFi reconnection...");

        if (connectWithSavedCredentials())
        {
            Serial.println("📶 Reconnection attempt started with saved credentials");
        }
        else
        {
            Serial.println("❌ Reconnection failed - no saved credentials available");
            setState(WiFiState::CREDENTIAL_NOT_FOUND);
        }
    }

    // ============================================================================
    // ACCESS POINT MODE
    // ============================================================================

    void WiFiManager::setupAP()
    {
        setState(WiFiState::AP_MODE_INIT);
        Serial.println("📶 Configuring device as WiFi Access Point...");

        // Set WiFi mode to Access Point
        WiFi.mode(WIFI_AP);

        // Generate device-specific credentials using MAC address
        String apSSID = GET_AP_SSID();         // Format: "CloudMouse-{device_id}"
        String apPassword = GET_AP_PASSWORD(); // MAC-based secure password

        // Create Access Point with generated credentials
        bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str());

        if (apStarted)
        {
            setState(WiFiState::AP_MODE);
            Serial.printf("✅ Access Point created successfully\n");
            Serial.printf("📶 Network Name: %s\n", apSSID.c_str());
            Serial.printf("📶 Password: %s\n", apPassword.c_str());
            Serial.printf("📶 IP Address: %s\n", WiFi.softAPIP().toString().c_str());
            Serial.println("📶 Device ready for configuration via web interface");
        }
        else
        {
            Serial.println("❌ Failed to create Access Point");
            setState(WiFiState::ERROR);
        }
    }

    void WiFiManager::stopAP()
    {
        Serial.println("📶 Stopping Access Point...");

        // Gracefully disconnect all clients and stop AP
        WiFi.softAPdisconnect(true);
        Serial.println("✅ Access Point stopped successfully");
    }

    bool WiFiManager::hasConnectedDevices()
    {
        // Check if any clients are connected to our Access Point
        return WiFi.softAPgetStationNum() > 0;
    }

    // ============================================================================
    // WPS (WiFi Protected Setup) SUPPORT
    // ============================================================================

    void WiFiManager::startWPS()
    {
        if (wpsStarted)
        {
            Serial.println("⚠️ WPS already active");
            return;
        }

        Serial.println("📶 Starting WPS (WiFi Protected Setup)...");
        Serial.println("📶 Press WPS button on your router within 2 minutes");

        // Configure WiFi for station mode
        WiFi.mode(WIFI_STA);

        // Initialize WPS with push-button configuration
        esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
        esp_wifi_wps_enable(&config);
        esp_wifi_wps_start(0);

        wpsStarted = true;
        setState(WiFiState::WPS_LISTENING);
    }

    void WiFiManager::stopWPS()
    {
        if (!wpsStarted)
        {
            Serial.println("⚠️ WPS not active");
            return;
        }

        Serial.println("📶 Stopping WPS mode...");

        // Disable WPS and return to normal operation
        esp_wifi_wps_disable();
        wpsStarted = false;

        Serial.println("✅ WPS stopped successfully");
    }

    // ============================================================================
    // STATE MANAGEMENT AND MONITORING
    // ============================================================================

    void WiFiManager::handleConnectionTimeout()
    {
        uint32_t connectionTime = millis() - connectionStartTime;

        // Check if connection attempt has exceeded timeout
        if (connectionTime > connectionTimeout)
        {
            Serial.printf("⏰ WiFi connection timeout after %d ms\n", connectionTime);
            Serial.println("📶 Connection attempt failed - consider AP mode for setup");
            setState(WiFiState::TIMEOUT);
        }
    }

    void WiFiManager::setState(WiFiState newState)
    {
        // Only log and update if state actually changes
        if (currentState != newState)
        {
            WiFiState oldState = currentState;
            currentState = newState;

            // Log state transition with descriptive information
            Serial.printf("📶 WiFi State Transition: %d → %d\n", (int)oldState, (int)newState);

            // Additional state-specific logging
            switch (newState)
            {
            case WiFiState::CONNECTING:
                Serial.println("📶 Status: Attempting WiFi connection...");
                break;
            case WiFiState::CONNECTED:
                Serial.println("📶 Status: WiFi connection established");
                break;
            case WiFiState::TIMEOUT:
                Serial.println("📶 Status: Connection timeout - setup required");
                break;
            case WiFiState::AP_MODE:
                Serial.println("📶 Status: Access Point mode active");
                break;
            case WiFiState::DISCONNECTED:
                Serial.println("📶 Status: WiFi disconnected");
                break;
            default:
                break;
            }
        }
    }

    void WiFiManager::saveCredentials(const String &ssid, const String &password)
    {
        // Save credentials to encrypted NVS storage for future use
        prefs.saveWiFiCredentials(ssid, password);
        Serial.printf("💾 WiFi credentials saved for network: %s\n", ssid.c_str());
    }

    // ============================================================================
    // STATUS QUERY IMPLEMENTATIONS
    // ============================================================================

    String WiFiManager::getLocalIP() const
    {
        if (isConnected())
        {
            // Return station mode IP address
            return WiFi.localIP().toString();
        }
        else if (isAPMode())
        {
            // Return Access Point IP address
            return WiFi.softAPIP().toString();
        }
        return ""; // No IP available
    }

    String WiFiManager::getSSID() const
    {
        if (isConnected())
        {
            // Return connected network name
            return WiFi.SSID();
        }
        else if (isAPMode())
        {
            // Return our Access Point name
            return DeviceID::getAPSSID();
        }
        return ""; // No network name available
    }

    int WiFiManager::getRSSI() const
    {
        if (isConnected())
        {
            // Return signal strength in dBm (negative values)
            return WiFi.RSSI();
        }

        return 0; // No signal information available
    }

    uint32_t WiFiManager::getConnectionTime() const
    {
        if (currentState == WiFiState::CONNECTING)
        {
            // Return elapsed connection attempt time
            return millis() - connectionStartTime;
        }
        return 0; // Not currently connecting
    }

    // ============================================================================
    // STATIC EVENT HANDLER
    // ============================================================================

    void WiFiManager::WiFiEventHandler(WiFiEvent_t event, arduino_event_info_t info)
    {
        // Ensure static instance is available for event processing
        if (!staticInstance)
        {
            Serial.println("⚠️ WiFi event received but no WiFiManager instance available");
            return;
        }

        // Process WiFi events and update state accordingly
        switch (event)
        {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            // Connection successful - IP address assigned
            Serial.println("✅ WiFi connection successful!");
            Serial.printf("📶 IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("📶 Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("📶 DNS: %s\n", WiFi.dnsIP().toString().c_str());
            Serial.printf("📶 Signal Strength: %d dBm\n", WiFi.RSSI());

            // Save successful credentials for future use
            staticInstance->saveCredentials(WiFi.SSID(), WiFi.psk());
            staticInstance->setState(WiFiState::CONNECTED);

            // Brief stabilization delay before additional services
            delay(1000);

            // Initialize NTP time synchronization
            Serial.println("⏰ Initializing network time synchronization...");
            CloudMouse::Utils::NTPManager::init();
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            // Connection lost or failed
            Serial.println("📶 WiFi connection lost");

            if (staticInstance->currentState == WiFiState::CONNECTING)
            {
                // Timeout will be handled by handleConnectionTimeout()
                Serial.println("📶 Connection attempt failed - timeout monitoring active");
            }
            else
            {
                // Unexpected disconnection from established connection
                Serial.println("📶 Unexpected disconnection - attempting automatic reconnection");
                staticInstance->setState(WiFiState::DISCONNECTED);
            }
            break;

        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            // WPS configuration successful
            Serial.println("✅ WPS configuration successful!");
            Serial.println("📶 Credentials received via WPS - attempting connection");

            staticInstance->stopWPS();
            staticInstance->setState(WiFiState::WPS_SUCCESS);

            // Begin connection with WPS-provided credentials
            WiFi.begin();
            break;

        case ARDUINO_EVENT_WPS_ER_FAILED:
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            // WPS failed or timed out
            Serial.println("❌ WPS configuration failed or timed out");
            Serial.println("📶 Consider manual configuration via Access Point mode");

            staticInstance->stopWPS();
            staticInstance->setState(WiFiState::WPS_FAILED);
            break;

        default:
            // Other WiFi events (informational only)
            Serial.printf("📶 WiFi Event: %d\n", event);
            break;
        }
    }
} // namespace CloudMouse::Network