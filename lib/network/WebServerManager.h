/**
 * CloudMouse SDK - Web Server Manager
 *
 * Captive portal web server for WiFi credential configuration during device setup.
 * Provides a responsive web interface for network selection and credential entry.
 *
 * Features:
 * - Automatic WiFi network scanning and display
 * - Responsive HTML interface with modern CSS styling
 * - Form-based credential collection with validation
 * - Integration with WiFiManager for connection handling
 * - Static file serving and error handling
 *
 * Usage:
 * 1. Initialize after setting up Access Point mode
 * 2. Call update() in main loop to handle client requests
 * 3. Web interface accessible at device IP (typically 192.168.4.1)
 * 4. Automatically saves credentials and initiates connection
 */

#pragma once
#include <WebServer.h>
#include <WiFi.h>
#include "WiFiManager.h"

namespace CloudMouse::Network
{
    class WebServerManager
    {
    public:
        /**
         * Constructor - requires WiFiManager reference for credential handling
         *
         * @param wifiMgr Reference to WiFiManager instance for connection management
         */
        WebServerManager(WiFiManager &wifiMgr);
        ~WebServerManager() = default;

        /**
         * Initialize web server and register route handlers
         * Automatically scans for available WiFi networks
         * Starts HTTP server on port 80
         */
        void init();

        /**
         * Process incoming HTTP requests
         * Should be called regularly in main loop when AP mode is active
         */
        void update();

        /**
         * Stop web server and free resources
         * Call when exiting AP mode or during shutdown
         */
        void stop();

        /**
         * Get current server status
         *
         * @return true if server is running and handling requests
         */
        bool isRunning() const { return serverRunning; }

        /**
         * Manually trigger network scan
         * Updates available networks list for web interface
         */
        void refreshNetworks() { scanNetworks(); }

    private:
        WebServer webServer;        // ESP32 web server instance (port 80)
        WiFiManager &wifiManager;   // Reference to WiFi connection manager
        String networkList;         // HTML options list of scanned networks
        bool serverRunning = false; // Server status flag

        // Static instance pointer for callback handlers
        static WebServerManager *instance;

        /**
         * Scan for available WiFi networks and build HTML options list
         * Includes SSID and signal strength (RSSI) for each network
         * Results stored in networkList for web page generation
         */
        void scanNetworks();

        /**
         * Generate complete HTML configuration page
         * Includes responsive CSS styling and JavaScript enhancements
         * Embeds scanned network list as select options
         *
         * @return Complete HTML page as String
         */
        String generateConfigPage();

        // Static HTTP request handlers (required for WebServer callback system)

        /**
         * Handle GET requests to root path "/"
         * Serves main configuration page with network selection form
         */
        static void handleRoot();

        /**
         * Handle POST requests to "/config" endpoint
         * Processes WiFi credential form submission
         * Validates input and initiates connection attempt
         * Returns success page with connection status
         */
        static void handleConfig();

        /**
         * Handle requests to undefined routes
         * Returns 404 error response
         */
        static void handleNotFound();
    };
};