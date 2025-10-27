/**
 * CloudMouse SDK - Web Server Manager Implementation
 *
 * Captive portal implementation for WiFi credential collection during device setup.
 * Provides responsive web interface with automatic network detection and modern UI.
 */

#include "./WebServerManager.h"
#include "../prefs/PreferencesManager.h"

namespace CloudMouse::Network
{
    // Static instance pointer for callback system
    WebServerManager *WebServerManager::instance = nullptr;

    WebServerManager::WebServerManager(WiFiManager &wifiMgr)
        : webServer(80), wifiManager(wifiMgr)
    {
        // Set static instance for callback handlers
        instance = this;
    }

    void WebServerManager::init()
    {
        Serial.println("🌐 Initializing WebServer...");

        // Scan for available WiFi networks to populate selection list
        scanNetworks();

        // Register HTTP route handlers
        webServer.on("/", handleRoot);                    // Main configuration page
        webServer.on("/config", HTTP_POST, handleConfig); // Credential submission endpoint
        webServer.onNotFound(handleNotFound);             // 404 handler for undefined routes

        // Start HTTP server on port 80
        webServer.begin();
        serverRunning = true;

        Serial.println("✅ WebServer started on port 80");
        Serial.println("🌐 Access configuration at: http://192.168.4.1");
    }

    void WebServerManager::update()
    {
        // Process incoming HTTP requests (non-blocking)
        // Should be called regularly in main loop
        webServer.handleClient();
    }

    void WebServerManager::stop()
    {
        webServer.stop();
        serverRunning = false;
        Serial.println("🌐 WebServer stopped");
    }

    void WebServerManager::scanNetworks()
    {
        Serial.println("🔍 Scanning WiFi networks...");

        // Perform WiFi network scan
        int networkCount = WiFi.scanNetworks();
        networkList = "";

        // Build HTML option elements for each discovered network
        for (int i = 0; i < networkCount; i++)
        {
            networkList += "<option value='" + WiFi.SSID(i) + "'>";
            networkList += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
            networkList += "</option>";
        }

        Serial.printf("✅ Found %d networks\n", networkCount);
    }

    String WebServerManager::generateConfigPage()
    {
        // Generate complete HTML page with embedded CSS and JavaScript
        // Uses modern responsive design with gradient styling
        // Includes form validation and user experience enhancements

        return R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CloudMouse - WiFi Configuration</title>
    <style>
        /* Reset and base styles */
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        
        /* Main container */
        .container {
            background: white;
            border-radius: 16px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 40px;
            max-width: 400px;
            width: 100%;
        }
        
        /* Logo and branding */
        .logo {
            text-align: center;
            margin-bottom: 30px;
        }
        .logo h1 {
            color: #333;
            font-size: 24px;
            font-weight: 600;
        }
        .logo p {
            color: #666;
            font-size: 14px;
            margin-top: 5px;
        }
        
        /* Form styling */
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
        }
        select, input[type="password"] {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e1e5e9;
            border-radius: 8px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        select:focus, input[type="password"]:focus {
            outline: none;
            border-color: #667eea;
        }
        
        /* Button styling */
        .btn-primary {
            width: 100%;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 14px;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        .btn-primary:hover {
            transform: translateY(-2px);
        }
        
        /* Info box styling */
        .info {
            background: #f8f9fa;
            border-radius: 8px;
            padding: 16px;
            margin-top: 20px;
            font-size: 14px;
            color: #666;
        }
        .qr-hint {
            text-align: center;
            margin-top: 20px;
            font-size: 12px;
            color: #999;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">
            <h1>🕐 CloudMouse</h1>
            <p>WiFi Configuration</p>
        </div>
        
        <!-- WiFi credential form -->
        <form action="/config" method="POST">
            <div class="form-group">
                <label for="ssid">WiFi Network:</label>
                <select name="ssid" id="ssid" required>
                    <option value="">Select a network...</option>
)rawliteral" + networkList +
               R"rawliteral(
                </select>
            </div>
            
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" name="password" id="password" 
                       placeholder="Enter WiFi password" required>
            </div>
            
            <button type="submit" class="btn-primary">
                🔗 Connect
            </button>
        </form>
        
        <!-- User guidance -->
        <div class="info">
            <strong>💡 Note:</strong><br>
            After connection, the device will restart automatically 
            and be ready for use.
        </div>
        
        <div class="qr-hint">
            Scanned QR code from device display? 📱
        </div>
    </div>
</body>
</html>
)rawliteral";
    }

    // ============================================================================
    // STATIC HTTP REQUEST HANDLERS
    // ============================================================================

    void WebServerManager::handleRoot()
    {
        if (!instance)
            return;

        // Serve main configuration page
        instance->webServer.send(200, "text/html", instance->generateConfigPage());
    }

    void WebServerManager::handleConfig()
    {
        if (!instance)
            return;

        // Validate form data presence
        if (instance->webServer.hasArg("ssid") && instance->webServer.hasArg("password"))
        {
            String ssid = instance->webServer.arg("ssid");
            String password = instance->webServer.arg("password");

            Serial.printf("🌐 WiFi credentials received: %s\n", ssid.c_str());

            // Generate immediate success response for user feedback
            String successPage = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connecting...</title>
    <style>
        body { 
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            text-align: center; 
            padding: 50px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .spinner { 
            border: 4px solid rgba(255,255,255,0.3);
            border-top: 4px solid white;
            border-radius: 50%;
            width: 50px;
            height: 50px;
            animation: spin 1s linear infinite;
            margin: 20px auto;
        }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
    </style>
</head>
<body>
    <h2>🔗 Connecting...</h2>
    <div class="spinner"></div>
    <p>Device is connecting to network <strong>)rawliteral" +
                                 ssid + R"rawliteral(</strong></p>
    <p>This page will close automatically.</p>
    <script>setTimeout(() => window.close(), 5000);</script>
</body>
</html>
)rawliteral";

            // Send immediate response to prevent browser timeout
            instance->webServer.send(200, "text/html", successPage);

            // Save credentials for future use
            instance->wifiManager.saveCredentials(ssid, password);

            // Brief delay to ensure response is sent
            delay(1000);

            // Attempt WiFi connection with provided credentials
            instance->wifiManager.connect(ssid.c_str(), password.c_str());
        }
        else
        {
            // Handle missing form data
            Serial.println("❌ Invalid form submission - missing SSID or password");
            instance->webServer.send(400, "text/plain", "Error: Missing SSID or password");
        }
    }

    void WebServerManager::handleNotFound()
    {
        if (!instance)
            return;

        // Handle requests to undefined routes
        instance->webServer.send(404, "text/plain", "Page not found");
    }
} // namespace CloudMouse::Network