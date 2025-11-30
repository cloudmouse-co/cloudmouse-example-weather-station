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
    SDK_LOGGER("🚀 Core initialization starting...");

    // Output device identification
    DeviceID::printDeviceInfo();

    // Initialize event communication system
    EventBus::instance().initialize();

    // Initialize app orchestrator
    if (appOrchestrator) {
      appOrchestrator->initialize();
    }

    // Start system in booting state (shows LED animation)
    setState(SystemState::BOOTING);

    SDK_LOGGER("🎬 Boot sequence started - LED animation active");
    SDK_LOGGER("✅ Core initialized successfully");
  }

  void Core::startUITask()
  {
    if (uiTaskHandle != nullptr)
    {
      SDK_LOGGER("🎮 UI Task already running");
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
      SDK_LOGGER("✅ UI Task running on Core 1 (30Hz)");

      // Start LED animation system
      if (ledManager)
      {
        ledManager->startAnimationTask();
      }
    }
    else
    {
      setState(SystemState::ERROR);
      SDK_LOGGER("❌ Failed to start UI Task!");
    }
  }

  void Core::start()
  {
    if (currentState != SystemState::READY)
    {
      SDK_LOGGER("❌ Core not ready to start!");
      return;
    }

    // ledManager->setRainbowState(true, 2);

    setState(SystemState::RUNNING);
    SDK_LOGGER("✅ System started - CloudMouse RUNNING");
  }

  // ============================================================================
  // STATE MANAGEMENT
  // ============================================================================

  void Core::setState(SystemState state)
  {
    if (currentState != state)
    {
      SDK_LOGGER("🔄 State transition: %d → %d\n", (int)currentState, (int)state);
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
    
    // update loop for app orchestrator
    if (appOrchestrator) {
      appOrchestrator->update();
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
      SDK_LOGGER("📡 WiFi required - starting connection process");

      if (wifi)
      {
        EventBus::instance().sendToUI(Event(EventType::DISPLAY_WIFI_CONNECTING));
        wifi->init();
      }
#else
      SDK_LOGGER("📡 WiFi optional - ready for operation");
      EventBus::instance().sendToUI(Event(EventType::DISPLAY_WAKE_UP));
      setState(SystemState::READY);
#endif

      if (appOrchestrator) {
        Event bootingCompleted(EventType::BOOTING_COMPLETE);
        appOrchestrator->processSDKEvent(bootingCompleted);
      }
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
        SDK_LOGGER("📡 WiFi: Attempting connection...");
        setState(SystemState::WIFI_CONNECTING);
        // Visual feedback: loading state
        if (ledManager)
        {
          ledManager->setLoadingState(true);
        }

        // Sending wifi connecting event to the app orchestrator
        if (appOrchestrator) {
          Event wifiConnected(EventType::WIFI_CONNECTING);
          appOrchestrator->processSDKEvent(wifiConnected);
        }
        break;

      case WiFiManager::WiFiState::CONNECTED:
      {
        SDK_LOGGER("✅ WiFi: Connected successfully!");
        String ssid = wifi->getSSID();
        String ip = wifi->getLocalIP();
        SDK_LOGGER("   Network: %s, IP: %s\n", ssid.c_str(), ip.c_str());

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

        // Sending wifi connected event to the app orchestrator
        if (appOrchestrator) {
          Event wifiConnected(EventType::WIFI_CONNECTED);
          appOrchestrator->processSDKEvent(wifiConnected);
        }

        setState(SystemState::READY);
      }
      break;

      case WiFiManager::WiFiState::CREDENTIAL_NOT_FOUND:
      case WiFiManager::WiFiState::TIMEOUT:
      case WiFiManager::WiFiState::ERROR:
        SDK_LOGGER("❌ WiFi: Connection failed - starting setup mode");

        // Sending wifi disconnected event to the app orchestrator
        if (appOrchestrator) {
          Event wifiConnected(EventType::WIFI_DISCONNECTED);
          appOrchestrator->processSDKEvent(wifiConnected);
        }

        if (wifi)
        {
          wifi->setupAP();
        }
        break;

      case WiFiManager::WiFiState::AP_MODE:
        SDK_LOGGER("📱 WiFi: Access Point mode active");
        setState(SystemState::WIFI_AP_MODE);

        if (webServer)
        {
          webServer->init();
          String apIP = wifi->getAPIP();
          String apSSID = wifi->getSSID();

          SDK_LOGGER("   AP Name: %s\n", apSSID.c_str());
          SDK_LOGGER("   Setup URL: http://%s\n", apIP.c_str());

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
        SDK_LOGGER("📱 Client connected - showing setup instructions");

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

      if (appOrchestrator) {
        appOrchestrator->processSDKEvent(event);
      }

      switch (event.type)
      {
      case EventType::ENCODER_ROTATION:
        handleEncoderRotation(event);
        break;

      case EventType::ENCODER_CLICK:
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
    SDK_LOGGER("🔄 Encoder rotation: %d steps\n", event.value);

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
    SDK_LOGGER("🖱️ Encoder clicked!");

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
    SDK_LOGGER("⏱️ Encoder long press detected!");

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

    SDK_LOGGER("🎮 UI Task started on Core 1");

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

    SDK_LOGGER("🏥 Health: Free=%d, Min=%d, Tasks=%d, Cycles=%d, Events=%d\n",
                  freeHeap, minFreeHeap, uxTaskGetNumberOfTasks(),
                  coordinationCycles, eventsProcessed);

    // Monitor UI task stack usage
    if (uiTaskHandle)
    {
      UBaseType_t uiStack = uxTaskGetStackHighWaterMark(uiTaskHandle);
      SDK_LOGGER("🎮 UI Task stack remaining: %d bytes\n", uiStack * sizeof(StackType_t));
    }

    // Monitor LED task stack usage
    if (ledManager && ledManager->getAnimationTaskHandle())
    {
      UBaseType_t ledStack = uxTaskGetStackHighWaterMark(ledManager->getAnimationTaskHandle());
      SDK_LOGGER("💡 LED Task stack remaining: %d bytes\n", ledStack * sizeof(StackType_t));

      // Auto-restart LED task if stack is critically low
      if (ledStack < 512)
      {
        SDK_LOGGER("⚠️ LED Task stack critically low - restarting");
        ledManager->restartAnimationTask();
      }
    }

    // Log event bus performance
    EventBus::instance().logStatus();

    // Memory warning
    if (freeHeap < 50000)
    {
      SDK_LOGGER("⚠️ LOW MEMORY WARNING!");
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

          SDK_LOGGER("\n💬 Command: '%s'\n", commandBuffer.c_str());

          // Device information query
          if (commandBuffer == "get uuid")
          {
            String uuid = GET_DEVICE_UUID();
            String deviceId = GET_DEVICE_ID();
            String mac = DeviceID::getMACAddress();

            SDK_LOGGER("\n📱 DEVICE_INFO_START");
            SDK_LOGGER("{");
            SDK_LOGGER("  \"uuid\": \"%s\",\n", uuid.c_str());
            SDK_LOGGER("  \"device_id\": \"%s\",\n", deviceId.c_str());
            SDK_LOGGER("  \"mac_address\": \"%s\",\n", mac.c_str());
            SDK_LOGGER("  \"pcb_version\": %d,\n", PCB_VERSION);
            SDK_LOGGER("  \"firmware_version\": \"%s\",\n", FIRMWARE_VERSION);
            SDK_LOGGER("  \"chip_model\": \"%s\",\n", ESP.getChipModel());
            SDK_LOGGER("  \"chip_revision\": %d\n", ESP.getChipRevision());
            SDK_LOGGER("}");
            SDK_LOGGER("📱 DEVICE_INFO_END\n");

            // System restart
          }
          else if (commandBuffer == "reboot")
          {
            SDK_LOGGER("🔄 Rebooting CloudMouse...");
            Serial.flush();
            delay(500);
            ESP.restart();

            // Factory reset
          }
          else if (commandBuffer == "hard reset")
          {
            SDK_LOGGER("🗑️ Factory reset - clearing all settings...");
            prefs.clearAll();
            SDK_LOGGER("✅ Settings cleared!");
            SDK_LOGGER("🔄 Rebooting...");
            Serial.flush();
            delay(500);
            ESP.restart();

            // Help system
          }
          else if (commandBuffer == "help")
          {
            SDK_LOGGER("\n📋 CloudMouse Commands:");
            SDK_LOGGER("  reboot      - Restart the device");
            SDK_LOGGER("  hard reset  - Factory reset (clear all settings)");
            SDK_LOGGER("  status      - Show system information");
            SDK_LOGGER("  get uuid    - Get device identification");
            SDK_LOGGER("  help        - Show this help\n");

            // System status
          }
          else if (commandBuffer == "status")
          {
            SDK_LOGGER("\n📊 CloudMouse Status:");
            SDK_LOGGER("  State: %d\n", (int)currentState);
            SDK_LOGGER("  Uptime: %lu seconds\n", millis() / 1000);
            SDK_LOGGER("  Free Heap: %d bytes\n", ESP.getFreeHeap());
            SDK_LOGGER("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
            SDK_LOGGER("  Coordination Cycles: %d\n", coordinationCycles);
            SDK_LOGGER("  Events Processed: %d\n", eventsProcessed);
            if (wifi)
            {
              SDK_LOGGER("  WiFi State: %d\n", (int)wifi->getState());
              if (wifi->isConnected())
              {
                SDK_LOGGER("  Network: %s\n", wifi->getSSID().c_str());
                SDK_LOGGER("  IP Address: %s\n", wifi->getLocalIP().c_str());
                SDK_LOGGER("  Signal: %d dBm\n", wifi->getRSSI());
              }
            }
            SDK_LOGGER("");
          }
          else
          {
            SDK_LOGGER("❌ Unknown command: '%s'\n", commandBuffer.c_str());
            SDK_LOGGER("   Type 'help' for available commands\n");
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