/**
 * CloudMouse SDK - Core System Implementation
 *
 * Main system controller that orchestrates all CloudMouse components.
 * Handles dual-core operation, event processing, and system lifecycle.
 */

#include "./Core.h"

namespace CloudMouse
{
  // ============================================================================
  // SYSTEM INITIALIZATION
  // ============================================================================

  void Core::initialize()
  {
    Serial.println("🚀 Core initialization starting...");

    // Output device identification
    DeviceID::printDeviceInfo();

    // Initialize event communication system
    EventBus::instance().initialize();

    // Start system in booting state (shows LED animation)
    setState(SystemState::BOOTING);

    Serial.println("🎬 Boot sequence started - LED animation active");
    Serial.println("✅ Core initialized successfully");
  }

  void Core::startUITask()
  {
    if (uiTaskHandle != nullptr)
    {
      Serial.println("🎮 UI Task already running");
      return;
    }

    // Create UI task on Core 1 for smooth 30Hz rendering
    xTaskCreatePinnedToCore(
        uiTaskFunction,
        "UI_Task",
        8192, // 8KB stack
        this, // Pass Core instance
        1,    // High priority for UI responsiveness
        &uiTaskHandle,
        1 // Pin to Core 1
    );

    if (uiTaskHandle)
    {
      Serial.println("✅ UI Task running on Core 1 (30Hz)");

      // Start LED animation system
      if (ledManager)
      {
        ledManager->startAnimationTask();
      }
    }
    else
    {
      setState(SystemState::ERROR);
      Serial.println("❌ Failed to start UI Task!");
    }
  }

  void Core::start()
  {
    if (currentState != SystemState::READY)
    {
      Serial.println("❌ Core not ready to start!");
      return;
    }

    setState(SystemState::RUNNING);
    Serial.println("✅ System started - CloudMouse RUNNING");
  }

  // ============================================================================
  // STATE MANAGEMENT
  // ============================================================================

  void Core::setState(SystemState state)
  {
    if (currentState != state)
    {
      Serial.printf("🔄 State transition: %d → %d\n", (int)currentState, (int)state);
      currentState = state;
      stateStartTime = millis();
    }
  }

  // ============================================================================
  // MAIN COORDINATION LOOP (Core 0 - 20Hz)
  // ============================================================================

  void Core::coordinationLoop()
  {
    // Handle boot sequence timing
    if (currentState == SystemState::BOOTING)
    {
      handleBootingState();
    }

    // WiFi management and state handling
    if (wifi)
    {
      wifi->update();
      handleWiFiConnection();
    }

    // Web server updates when in AP mode
    if (wifi && wifi->getState() == WiFiManager::WiFiState::AP_MODE && webServer)
    {
      webServer->update();
    }

    // Auto-transition to running state when ready
    if (currentState == SystemState::READY)
    {
      start();
    }

    // Update weather service if available and WiFi connected
    if (weatherService && currentState == SystemState::RUNNING) {
        weatherService->update();
    }

    // Process user commands and system events
    processSerialCommands();
    processEvents();

    coordinationCycles++;

    // System health monitoring (every 5 seconds)
    if (millis() - lastHealthCheck > 5000)
    {
      checkHealth();
      lastHealthCheck = millis();
    }
  }

  // ============================================================================
  // BOOT SEQUENCE HANDLER
  // ============================================================================

  void Core::handleBootingState()
  {
    // Wait for 4 second boot animation to complete
    if (millis() >= 4000)
    {
      setState(SystemState::INITIALIZING);

      #if WIFI_REQUIRED
            Serial.println("📡 WiFi required - starting connection process");

            if (wifi)
            {
              EventBus::instance().sendToUI(Event(EventType::DISPLAY_WIFI_CONNECTING));
              wifi->init();
            }
      #else
            Serial.println("📡 WiFi optional - ready for operation");
            EventBus::instance().sendToUI(Event(EventType::DISPLAY_WAKE_UP));
            setState(SystemState::READY);
      #endif
    }
  }

  // ============================================================================
  // WIFI CONNECTION HANDLER
  // ============================================================================

  void Core::handleWiFiConnection()
  {
    static WiFiManager::WiFiState lastWiFiState = WiFiManager::WiFiState::DISCONNECTED;
    WiFiManager::WiFiState currentWiFiState = wifi->getState();

    // Process WiFi state changes
    if (currentWiFiState != lastWiFiState)
    {
      lastWiFiState = currentWiFiState;

      switch (currentWiFiState)
      {
      case WiFiManager::WiFiState::CONNECTING:
        Serial.println("📡 WiFi: Attempting connection...");
        setState(SystemState::WIFI_CONNECTING);
        // Visual feedback: loading state
        if (ledManager)
        {
          ledManager->setLoadingState(true);
        }
        break;

      case WiFiManager::WiFiState::CONNECTED:
      {
        Serial.println("✅ WiFi: Connected successfully!");
        String ssid = wifi->getSSID();
        String ip = wifi->getLocalIP();
        Serial.printf("   Network: %s, IP: %s\n", ssid.c_str(), ip.c_str());

        // Visual feedback: green LED flash
        if (ledManager)
        {
          ledManager->setLoadingState(false);
          ledManager->flashColor(0, 255, 0, 255, 500);
        }

        // Initialize weather service and show weather UI
        if (weatherService)
        {
            Serial.println("🌤️ Starting weather service...");
            weatherService->begin();  // Fetch initial data
            
            // Show weather UI
            Event weatherEvent(EventType::DISPLAY_UPDATE);
            EventBus::instance().sendToUI(weatherEvent);
        }
        else
        {
            // Fallback to hello screen if no weather service
            Event helloEvent(EventType::DISPLAY_WAKE_UP);
            EventBus::instance().sendToUI(helloEvent);
        }

        setState(SystemState::READY);
      }
      break;

      case WiFiManager::WiFiState::CREDENTIAL_NOT_FOUND:
      case WiFiManager::WiFiState::TIMEOUT:
      case WiFiManager::WiFiState::ERROR:
        Serial.println("❌ WiFi: Connection failed - starting setup mode");

        if (wifi)
        {
          wifi->setupAP();
        }
        break;

      case WiFiManager::WiFiState::AP_MODE:
        Serial.println("📱 WiFi: Access Point mode active");
        setState(SystemState::WIFI_AP_MODE);

        if (webServer)
        {
          webServer->init();
          String apIP = wifi->getAPIP();
          String apSSID = wifi->getSSID();

          Serial.printf("   AP Name: %s\n", apSSID.c_str());
          Serial.printf("   Setup URL: http://%s\n", apIP.c_str());

          // Show AP setup screen with QR code
          Event apEvent(EventType::DISPLAY_WIFI_AP_MODE);
          apEvent.setStringData((apSSID + "|" + apIP).c_str());
          EventBus::instance().sendToUI(apEvent);

          // Visual feedback: blue LED flash
          if (ledManager)
          {
            ledManager->flashColor(0, 100, 255, 255, 1000);
          }
        }
        break;

      default:
        break;
      }
    }

    // Monitor for clients connecting to our AP
    if (currentWiFiState == WiFiManager::WiFiState::AP_MODE && wifi)
    {
      static bool clientWasConnected = false;
      bool clientIsConnected = wifi->hasAPClient();

      if (clientIsConnected && !clientWasConnected)
      {
        Serial.println("📱 Client connected - showing setup instructions");

        String setupURL = "http://" + wifi->getAPIP() + "/setup";

        // Display setup URL with QR code
        Event setupEvent(EventType::DISPLAY_WIFI_SETUP_URL);
        setupEvent.setStringData(setupURL.c_str());
        EventBus::instance().sendToUI(setupEvent);

        // Visual feedback: green LED flash
        if (ledManager)
        {
          ledManager->flashColor(0, 255, 0, 255, 300);
        }
      }

      clientWasConnected = clientIsConnected;
    }
  }

  // ============================================================================
  // EVENT PROCESSING SYSTEM
  // ============================================================================

  void Core::processEvents()
  {
    Event event;

    // Process all pending events from UI task
    while (EventBus::instance().receiveFromUI(event, 0))
    {
      eventsProcessed++;

      switch (event.type)
      {
      case EventType::ENCODER_ROTATION:
        handleEncoderRotation(event);
        break;

      case EventType::ENCODER_CLICK:
        handleWeatherUpdate();

        handleEncoderClick(event);
        break;

      case EventType::ENCODER_LONG_PRESS:
        handleEncoderLongPress(event);
        break;

      default:
        // Unhandled event type
        break;
      }
    }
  }

  void Core::handleEncoderRotation(const Event &event)
  {
    Serial.printf("🔄 Encoder rotation: %d steps\n", event.value);

    // Activate LED feedback
    if (ledManager)
    {
      ledManager->activate();
    }

    // Forward to UI system
    EventBus::instance().sendToUI(event);
  }

  void Core::handleEncoderClick(const Event &event)
  {
    Serial.println("🖱️ Encoder clicked!");

    // Visual feedback: green LED flash
    if (ledManager)
    {
      ledManager->flashColor(0, 255, 0, 255, 200);
    }

    // Audio feedback
    SimpleBuzzer::buzz();

    // Forward to UI system
    EventBus::instance().sendToUI(event);
  }

  void Core::handleEncoderLongPress(const Event &event)
  {
    Serial.println("⏱️ Encoder long press detected!");

    // Visual feedback: orange LED flash
    if (ledManager)
    {
      ledManager->flashColor(255, 165, 0, 255, 500);
    }

    // Audio feedback: error pattern
    SimpleBuzzer::error();

    // Forward to UI system
    EventBus::instance().sendToUI(event);
  }

  void Core::handleWeatherUpdate()
  {
      if (!weatherService) return;
      
      const WeatherData& weather = weatherService->getCurrentWeather();
      const DayForecast* forecast = weatherService->getForecast();
      
      if (!weather.isValid) {
          Serial.println("⚠️ Weather data not valid yet");
          return;
      }
      
      // Send current weather to UI
      Event weatherEvent(EventType::WEATHER_DATA_CURRENT);
      
      // Pack data into string: "temp|humidity|windSpeed|weatherCode|condition"
      String data = String(weather.temperature, 1) + "|" +
                    String(weather.humidity) + "|" +
                    String(weather.windSpeed, 1) + "|" +
                    String(weather.weatherCode) + "|" +
                    weather.condition;
      
      weatherEvent.setStringData(data.c_str());
      EventBus::instance().sendToUI(weatherEvent);
      
      // Send forecast data (3 days)
      if (weatherService->isForecastValid()) {
          for (int i = 0; i < 3; i++) {
              Event forecastEvent(EventType::WEATHER_DATA_FORECAST);
              forecastEvent.value = i; // Day index
              
              // Pack: "date|tempMax|tempMin|weatherCode"
              String forecastData = forecast[i].date + "|" +
                                    String(forecast[i].tempMax, 1) + "|" +
                                    String(forecast[i].tempMin, 1) + "|" +
                                    String(forecast[i].weatherCode);
              
              forecastEvent.setStringData(forecastData.c_str());
              EventBus::instance().sendToUI(forecastEvent);
          }
      }
      
      Serial.println("📤 Weather data sent to UI");
  }

  // ============================================================================
  // UI TASK (Core 1 - 30Hz)
  // ============================================================================

  void Core::uiTaskFunction(void *param)
  {
    Core *core = static_cast<Core *>(param);
    core->runUITask();
  }

  void Core::runUITask()
  {
    TickType_t lastWake = xTaskGetTickCount();

    Serial.println("🎮 UI Task started on Core 1");

    while (true)
    {
      // Read encoder input
      if (encoder)
      {
        encoder->update();

        // Handle rotation
        int movement = encoder->getMovement();
        if (movement != 0)
        {
          Event rotationEvent(EventType::ENCODER_ROTATION, movement);
          EventBus::instance().sendToMain(rotationEvent);
        }

        // Handle click
        if (encoder->getClicked())
        {
          Event clickEvent(EventType::ENCODER_CLICK);
          EventBus::instance().sendToMain(clickEvent);
        }

        // Handle long press
        if (encoder->getLongPressed())
        {
          Event longPressEvent(EventType::ENCODER_LONG_PRESS);
          EventBus::instance().sendToMain(longPressEvent);
        }
      }

      // Update display rendering
      if (display)
      {
        display->update();
      }

      // Maintain 30Hz update rate (33ms intervals)
      vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(33));
    }
  }

  // ============================================================================
  // SYSTEM HEALTH MONITORING
  // ============================================================================

  void Core::checkHealth()
  {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();

    Serial.printf("🏥 Health: Free=%d, Min=%d, Tasks=%d, Cycles=%d, Events=%d\n",
                  freeHeap, minFreeHeap, uxTaskGetNumberOfTasks(),
                  coordinationCycles, eventsProcessed);

    // Monitor UI task stack usage
    if (uiTaskHandle)
    {
      UBaseType_t uiStack = uxTaskGetStackHighWaterMark(uiTaskHandle);
      Serial.printf("🎮 UI Task stack remaining: %d bytes\n", uiStack * sizeof(StackType_t));
    }

    // Monitor LED task stack usage
    if (ledManager && ledManager->getAnimationTaskHandle())
    {
      UBaseType_t ledStack = uxTaskGetStackHighWaterMark(ledManager->getAnimationTaskHandle());
      Serial.printf("💡 LED Task stack remaining: %d bytes\n", ledStack * sizeof(StackType_t));

      // Auto-restart LED task if stack is critically low
      if (ledStack < 512)
      {
        Serial.println("⚠️ LED Task stack critically low - restarting");
        ledManager->restartAnimationTask();
      }
    }

    // Log event bus performance
    EventBus::instance().logStatus();

    // Memory warning
    if (freeHeap < 50000)
    {
      Serial.println("⚠️ LOW MEMORY WARNING!");
    }
  }

  // ============================================================================
  // SERIAL COMMAND INTERFACE
  // ============================================================================

  void Core::processSerialCommands()
  {
    static String commandBuffer = "";

    // Build command from serial input
    while (Serial.available() > 0)
    {
      char c = Serial.read();

      if (c == '\n' || c == '\r')
      {
        // Process complete command
        if (commandBuffer.length() > 0)
        {
          commandBuffer.trim();
          commandBuffer.toLowerCase();

          Serial.printf("\n💬 Command: '%s'\n", commandBuffer.c_str());

          // Device information query
          if (commandBuffer == "get uuid")
          {
            String uuid = GET_DEVICE_UUID();
            String deviceId = GET_DEVICE_ID();
            String mac = DeviceID::getMACAddress();

            Serial.println("\n📱 DEVICE_INFO_START");
            Serial.println("{");
            Serial.printf("  \"uuid\": \"%s\",\n", uuid.c_str());
            Serial.printf("  \"device_id\": \"%s\",\n", deviceId.c_str());
            Serial.printf("  \"mac_address\": \"%s\",\n", mac.c_str());
            Serial.printf("  \"pcb_version\": %d,\n", PCB_VERSION);
            Serial.printf("  \"firmware_version\": \"%s\",\n", FIRMWARE_VERSION);
            Serial.printf("  \"chip_model\": \"%s\",\n", ESP.getChipModel());
            Serial.printf("  \"chip_revision\": %d\n", ESP.getChipRevision());
            Serial.println("}");
            Serial.println("📱 DEVICE_INFO_END\n");

            // System restart
          }
          else if (commandBuffer == "reboot")
          {
            Serial.println("🔄 Rebooting CloudMouse...");
            Serial.flush();
            delay(500);
            ESP.restart();

            // Factory reset
          }
          else if (commandBuffer == "hard reset")
          {
            Serial.println("🗑️ Factory reset - clearing all settings...");
            prefs.clearAll();
            Serial.println("✅ Settings cleared!");
            Serial.println("🔄 Rebooting...");
            Serial.flush();
            delay(500);
            ESP.restart();

            // Help system
          }
          else if (commandBuffer == "help")
          {
            Serial.println("\n📋 CloudMouse Commands:");
            Serial.println("  reboot      - Restart the device");
            Serial.println("  hard reset  - Factory reset (clear all settings)");
            Serial.println("  status      - Show system information");
            Serial.println("  get uuid    - Get device identification");
            Serial.println("  help        - Show this help\n");

            // System status
          }
          else if (commandBuffer == "status")
          {
            Serial.println("\n📊 CloudMouse Status:");
            Serial.printf("  State: %d\n", (int)currentState);
            Serial.printf("  Uptime: %lu seconds\n", millis() / 1000);
            Serial.printf("  Free Heap: %d bytes\n", ESP.getFreeHeap());
            Serial.printf("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
            Serial.printf("  Coordination Cycles: %d\n", coordinationCycles);
            Serial.printf("  Events Processed: %d\n", eventsProcessed);
            if (wifi)
            {
              Serial.printf("  WiFi State: %d\n", (int)wifi->getState());
              if (wifi->isConnected())
              {
                Serial.printf("  Network: %s\n", wifi->getSSID().c_str());
                Serial.printf("  IP Address: %s\n", wifi->getLocalIP().c_str());
                Serial.printf("  Signal: %d dBm\n", wifi->getRSSI());
              }
            }
            Serial.println();
          }
          else
          {
            Serial.printf("❌ Unknown command: '%s'\n", commandBuffer.c_str());
            Serial.println("   Type 'help' for available commands\n");
          }

          commandBuffer = "";
        }
      }
      else
      {
        commandBuffer += c;
      }
    }
  }

} // namespace CloudMouse