/**
 * CloudMouse SDK - Device Configuration Header
 *
 * Central configuration hub for hardware variants, firmware versioning, and device-specific
 * settings. Provides compile-time configuration management with PCB version compatibility,
 * network settings, and device identification integration.
 *
 * Configuration Categories:
 * - Hardware PCB version selection with power management implications
 * - Firmware version tracking for OTA updates and compatibility
 * - WiFi network configuration and Access Point settings
 * - Device identification macros with automatic ID generation
 * - Service endpoint configuration for web-based setup
 *
 * PCB Version Management:
 * - Version 4: Legacy power logic (inverted enable signals)
 * - Version 5: Updated power logic (normal enable signals)
 * - Compile-time selection prevents hardware damage from incorrect logic
 * - Affects display power management and potentially other peripherals
 *
 * Device Identity System:
 * - Automatic device ID generation based on MAC address
 * - Unique UUID creation for cloud service registration
 * - Secure Access Point credentials derived from hardware ID
 * - Consistent device identification across firmware updates
 *
 * Network Configuration:
 * - WiFi requirement flags for deployment flexibility
 * - Access Point configuration for device setup mode
 * - Web service endpoints for configuration interface
 * - Standardized URLs for user setup experience
 *
 * Usage in SDK:
 * - Include this header in all hardware-dependent modules
 * - Use macros instead of hardcoded values for flexibility
 * - PCB_VERSION affects power management logic in display drivers
 * - Device ID macros provide consistent identification across modules
 * - Configuration changes require recompilation for safety
 */


#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "../utils/DeviceID.h"

namespace CloudMouse
{
// ============================================================================
// HARDWARE PCB VERSION CONFIGURATION
// ============================================================================

/**
 * PCB Hardware Version Selection
 *
 * Critical configuration that affects power management logic and pin behavior.
 * Must match the physical hardware to prevent damage from incorrect power sequencing.
 *
 * Version Differences:
 * - PCB v4: Inverted power enable logic (LOW = power on)
 * - PCB v5: Normal power enable logic (HIGH = power on)
 *
 * Affected Components:
 * - Display power management (TFT_PWR pin behavior)
 * - Potentially other power-controlled peripherals
 *
 * IMPORTANT: Verify PCB version before deployment to prevent hardware damage
 */
// #define PCB_VERSION 4 // Current hardware: PCB version 4
#define PCB_VERSION 5        // Uncomment for PCB version 5 hardware

// ============================================================================
// FIRMWARE VERSION MANAGEMENT
// ============================================================================

/**
 * Firmware Version String
 *
 * Semantic versioning for firmware releases and compatibility tracking.
 * Used for OTA update validation, diagnostic reporting, and version display.
 *
 * Format: MAJOR.MINOR.PATCH-PRERELEASE
 * - MAJOR: Breaking changes, incompatible API modifications
 * - MINOR: New features, backward-compatible additions
 * - PATCH: Bug fixes, security updates, minor improvements
 * - PRERELEASE: alpha, beta, rc (release candidate)
 *
 * Version History Integration:
 * - Displayed in device information screens
 * - Transmitted in diagnostic and telemetry data
 * - Used by OTA update system for compatibility validation
 * - Logged in system startup messages for debugging
 */
#define FIRMWARE_VERSION "3.0.0-alpha"

// ============================================================================
// NETWORK AND CONNECTIVITY CONFIGURATION
// ============================================================================

/**
 * WiFi Requirement Flag
 *
 * Determines whether WiFi connectivity is mandatory for device operation.
 * Affects startup behavior and error handling for network failures.
 *
 * true:  Device requires WiFi for full operation (cloud features, updates)
 * false: Device can operate offline (local-only functionality)
 *
 * Impact on Behavior:
 * - Startup sequence (WiFi initialization priority)
 * - Error handling (network failure responses)
 * - Feature availability (cloud vs local features)
 * - Power management (WiFi radio control)
 */
#define WIFI_REQUIRED true

// ============================================================================
// DEVICE IDENTIFICATION SYSTEM
// ============================================================================

/**
 * Device ID Generation Macros
 *
 * Provides consistent device identification across all SDK modules.
 * Based on ESP32 MAC address for hardware-tied unique identification.
 *
 * Usage Pattern:
 * - Use macros instead of direct DeviceID calls for consistency
 * - Device ID remains constant across firmware updates
 * - Unique per physical device for cloud service registration
 * - Human-readable format for debugging and support
 */

/**
 * Get unique device identifier string
 *
 * @return Short device ID derived from MAC address (e.g., "b126aaaf")
 *
 * Applications:
 * - Device registration with cloud services
 * - Local device identification in multi-device environments
 * - Diagnostic logging and support ticket correlation
 * - Default Access Point SSID generation
 */
#define GET_DEVICE_ID() DeviceID::getDeviceID()

/**
 * Get full device UUID string
 *
 * @return Complete UUID for device (e.g., "CloudMouse-b126aaaf-uuid")
 *
 * Applications:
 * - Comprehensive device registration
 * - Cloud service authentication
 * - Inter-device communication identification
 * - Detailed system logging and analytics
 */
#define GET_DEVICE_UUID() DeviceID::getDeviceUUID()

// ============================================================================
// ACCESS POINT CONFIGURATION
// ============================================================================

/**
 * Access Point Credential Generation
 *
 * Provides device-specific WiFi Access Point configuration for setup mode.
 * Credentials are derived from device hardware ID for security and uniqueness.
 *
 * Security Features:
 * - Unique SSID per device prevents conflicts in multi-device environments
 * - Hardware-derived password provides reasonable security for setup process
 * - Credentials remain consistent across firmware updates for user convenience
 */

/**
 * Get Access Point SSID
 *
 * @return Device-specific SSID (e.g., "CloudMouse-b126aaaf")
 *
 * Format: "CloudMouse-{device_id}"
 * - Clearly identifies device type and instance
 * - Unique per device to prevent SSID conflicts
 * - Human-readable for easy identification during setup
 */
#define GET_AP_SSID() DeviceID::getAPSSID()

/**
 * Get Access Point Password
 *
 * @return Secure password derived from device MAC address
 *
 * Security Characteristics:
 * - Generated from hardware MAC address for uniqueness
 * - Sufficient complexity for WPA2/WPA3 protection
 * - Consistent per device for user convenience
 * - Cannot be easily guessed without device access
 *
 * Note: Uses secure password generation (not the simple version)
 */
#define GET_AP_PASSWORD() DeviceID::getAPPasswordSecure()

// ============================================================================
// WEB SERVICE CONFIGURATION
// ============================================================================

/**
 * WiFi Configuration Service URL
 *
 * Standard URL for web-based device configuration interface.
 * Accessible when device is in Access Point mode for initial setup.
 *
 * Service Features:
 * - WiFi network selection and credential entry
 * - Device configuration parameter adjustment
 * - Firmware update initiation (if supported)
 * - System status and diagnostic information
 *
 * Access Method:
 * 1. Connect to device Access Point using credentials above
 * 2. Navigate to this URL in web browser
 * 3. Follow configuration wizard for setup completion
 *
 * Technical Details:
 * - Standard Access Point gateway address (192.168.4.1)
 * - HTTP protocol for broad device compatibility
 * - Captive portal detection for automatic redirection
 */
#define WIFI_CONFIG_SERVICE "http://192.168.4.1/"

/**
 * Device manufacturer
 * 
 * Applications:
 * - Used for BLE connection setup
 */
#define DEVICE_MANUFACTURER "Cloudmouse"

// ============================================================================
// DEBUGGING CONFIGURATION
// ============================================================================

/**
 * Show LVGL performance monitor
 * 
 * If true a performance monitor will be showed on the top right corner of display
 * showing FPS and CPU usage.
 * 
 * Applications:
 * - Used for checking animations and UI performance
 */
#define SHOW_LVGL_PERFORMANCE_MONITOR true

/**
 * APP logging flag
 * 
 * if `true` APP_LOGGER() logging is enabled with [APP] namespace
 * if `false` or not defined, APP_LOGGER() is disabled.
 */
#define APP_DEBUGGER_ACTIVE true

/**
 * SDK logging flag
 * 
 * if `true` SDK_LOGGER("") logging is enabled with [SDK] namespace
 * if `false` or not defined, SDK_LOGGER("") is disabled.
 */
#define SDK_DEBUGGER_ACTIVE true

// ============================================================================
// CONFIGURATION VALIDATION
// ============================================================================

/**
 * Compile-time Configuration Validation
 *
 * Ensures configuration consistency and prevents common deployment errors.
 * Validates PCB version selection and required dependency availability.
 */

// Helper macros for string conversion
// #define STRINGIFY(x) #x
// #define XSTRINGIFY(x) STRINGIFY(x)

// Validate PCB version selection
#if !defined(PCB_VERSION) || (PCB_VERSION != 4 && PCB_VERSION != 5)
#error "PCB_VERSION must be defined as either 4 or 5. Check your hardware version!"
#endif

// Validate firmware version is defined
#ifndef FIRMWARE_VERSION
#error "FIRMWARE_VERSION must be defined as a valid version string"
#endif

// Configuration summary for compile-time verification
// #pragma message "CloudMouse SDK Configuration:"
// #pragma message "  PCB Version: " XSTRINGIFY(PCB_VERSION)
// #pragma message "  Firmware: " FIRMWARE_VERSION
// #pragma message "  WiFi Required: " XSTRINGIFY(WIFI_REQUIRED)
}
#endif // DEVICE_CONFIG_H