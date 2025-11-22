/**
 * CloudMouse SDK - Core System
 *
 * The Core is the heart of the CloudMouse SDK, providing:
 * - Dual-core task management (UI on Core 1, Logic on Core 0)
 * - Event-driven architecture with hardware abstraction
 * - System state management and lifecycle control
 * - Component registration and coordination
 *
 * Architecture:
 * - Core 0: Main coordination, WiFi, event processing, system health
 * - Core 1: UI rendering, encoder input, display updates (30Hz)
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "EventBus.h"
#include "Events.h"
#include "../prefs/PreferencesManager.h"
#include "../hardware/LEDManager.h"
#include "../hardware/EncoderManager.h"
#include "../hardware/DisplayManager.h"
#include "../hardware/SimpleBuzzer.h"
#include "../network/WebServerManager.h"
#include "../services/WeatherService.h"


namespace CloudMouse
{

  using namespace Hardware;
  using namespace Network;

  /**
   * System state machine for CloudMouse lifecycle management
   */
  enum class SystemState
  {
    BOOTING,      // Initial boot animation (3.5s)
    INITIALIZING, // Hardware initialization

    // WiFi connection states
    WIFI_CONNECTING, // Attempting WiFi connection
    WIFI_CONNECTED,  // Successfully connected to WiFi
    WIFI_AP_MODE,    // Running as Access Point for setup
    WIFI_ERROR,      // WiFi connection failed

    // Operational states
    READY,   // All systems ready, waiting to start
    RUNNING, // Normal operation mode

    // Error handling
    ERROR // System error state
  };

  /**
   * Core System Controller
   *
   * Singleton class that manages the entire CloudMouse system.
   * Coordinates hardware components, manages dual-core operation,
   * and provides event-driven communication between subsystems.
   */
  class Core
  {
  public:
    // Singleton access
    static Core &instance()
    {
      static Core core;
      return core;
    }

    // System lifecycle management
    void initialize();  // Initialize core systems
    void start();       // Start normal operation
    void startUITask(); // Launch UI task on Core 1

    // Main coordination loop (runs on Core 0 at 20Hz)
    void coordinationLoop();

    // Hardware component registration
    void setEncoder(EncoderManager *encoder) { this->encoder = encoder; }
    void setDisplay(DisplayManager *display) { this->display = display; }
    void setWiFi(WiFiManager *wifi) { this->wifi = wifi; }
    void setWebServer(WebServerManager *webServer) { this->webServer = webServer; }
    void setLEDManager(LEDManager *ledManager) { this->ledManager = ledManager; }

    // service component registrations
    void setWeatherService(WeatherService* service) { this->weatherService = service; }
    
    // Hardware components getters
    EncoderManager* getEncoder() const { return encoder; }
    DisplayManager* getDisplay() const { return display; }
    WiFiManager* getWiFi() const { return wifi; }
    WebServerManager* getWebServer() const { return webServer; }
    LEDManager* getLEDManager() const { return ledManager; }

    // State management
    SystemState getState() const { return currentState; }
    void setState(SystemState state);

  private:
    // Singleton pattern enforcement
    Core() = default;
    ~Core() = default;
    Core(const Core &) = delete;
    Core &operator=(const Core &) = delete;

    // weather service component reference
    WeatherService* weatherService = nullptr;

    // System state tracking
    SystemState currentState = SystemState::BOOTING;
    uint32_t stateStartTime = 0;

    // Configuration
    bool wifiRequired = true;

    // Hardware component references
    EncoderManager *encoder = nullptr;
    DisplayManager *display = nullptr;
    WiFiManager *wifi = nullptr;
    WebServerManager *webServer = nullptr;
    LEDManager *ledManager = nullptr;

    // System services
    PreferencesManager prefs;
    TaskHandle_t uiTaskHandle = nullptr;

    // Performance monitoring
    uint32_t coordinationCycles = 0;
    uint32_t eventsProcessed = 0;
    uint32_t lastHealthCheck = 0;

    // FreeRTOS task functions
    static void uiTaskFunction(void *param);
    void runUITask();

    // State machine handlers
    void handleBootingState();
    void handleWiFiConnection();

    // Event processing system
    void processEvents();
    void processSerialCommands();
    void handleEncoderRotation(const Event &event);
    void handleEncoderClick(const Event &event);
    void handleEncoderLongPress(const Event &event);
    void handleWeatherUpdate();

    // System health monitoring
    void checkHealth();
  };

} // namespace CloudMouse