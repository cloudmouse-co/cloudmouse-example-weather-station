// bridge.cpp - include per compatibilità Arduino IDE
#include "lib/core/Core.cpp"
#include "lib/core/EventBus.cpp"
#include "lib/hardware/DisplayManager.cpp"
#include "lib/hardware/EncoderManager.cpp"
#include "lib/hardware/LEDManager.cpp"
#include "lib/network/WebServerManager.cpp"
#include "lib/network/WiFiManager.cpp"
#include "lib/utils/NTPManager.cpp"
#include "lib/utils/QRCodeManager.cpp"
#include "lib/prefs/PreferencesManager.cpp"

// application 
#include "lib/ui/WeatherUI.cpp"
#include "lib/services/WeatherService.cpp"
