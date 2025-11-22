/**
 * CloudMouse SDK - Bluetooth Manager
 *
 * Network layer abstraction for BLE connection management.
 * Handles BLE stack initialization, pairing, and connection lifecycle.
 *
 * IMPORTANT: This is ONLY the connection layer!
 * For application functionality (sending keys, media commands), use getBleKeyboard() to access the BleKeyboard instance.
 *
 * Features:
 * - BLE stack initialization and lifecycle management
 * - Automatic pairing and connection handling
 * - Event-driven state machine for connection lifecycle
 * - Exposes BleKeyboard instance for application layer
 *
 * Architecture:
 * - Network Layer: BluetoothManager (connection only)
 * - Application Layer: BleKeyboard instance (commands, keys, media)
 * - Similar pattern to WiFiManager exposing WiFi class
 *
 * State Machine:
 * IDLE → INITIALIZING → ADVERTISING → CONNECTED (success path)
 *                    ↘ DISCONNECTED (connection lost)
 *
 * Usage:
 * 1. Create instance and call init() during system startup
 * 2. Call update() regularly in main loop for state monitoring
 * 3. Use getBleKeyboard() to access BleKeyboard for application commands
 * 4. Monitor connection state via isConnected()
 *
 * Example:
 *   BluetoothManager bt;
 *   bt.init();
 *
 *   if (bt.isConnected()) {
 *       bt.getBleKeyboard()->write(KEY_MEDIA_VOLUME_UP);
 *   }
 */

#pragma once
#include <Arduino.h>
#include <BleKeyboard.h>
#include "../utils/DeviceID.h"
#include "../config/DeviceConfig.h"

using namespace CloudMouse::Utils;

namespace CloudMouse::Network
{
    /**
     * Bluetooth connection state enumeration
     * Represents all possible states in the BLE lifecycle
     */
    enum class BluetoothState
    {
        IDLE,         // BLE not initialized
        INITIALIZING, // BLE stack starting up
        ADVERTISING,  // Broadcasting, waiting for connection
        CONNECTED,    // Device paired and connected
        DISCONNECTED, // Was connected, now disconnected
        ERROR         // Error state
    };

    /**
     * Bluetooth Manager
     *
     * Network layer abstraction for BLE connection management.
     * Exposes BleKeyboard instance for application layer usage.
     */
    class BluetoothManager
    {
    public:
        /**
         * Constructor - prepares Bluetooth manager instance
         * Initializes state but does not start BLE stack
         */
        BluetoothManager();
        ~BluetoothManager() = default;

        // ========================================================================
        // LIFECYCLE MANAGEMENT
        // ========================================================================

        /**
         * Initialize Bluetooth and start advertising
         * Configures BLE HID device and begins advertising for pairing
         *
         * Device name format: "CloudMouse-XXXXXXXX" (using MAC address)
         * Manufacturer: "CloudMouse"
         *
         * Call once during system initialization
         */
        void init();

        /**
         * Update Bluetooth connection state
         * Monitors connection changes and updates internal state
         * Call regularly in main loop (every 10-50ms recommended)
         */
        void update();

        /**
         * Shutdown Bluetooth and free resources
         * Disconnects any active connection and stops advertising
         */
        void shutdown();

        // ========================================================================
        // CONNECTION STATUS
        // ========================================================================

        /**
         * Check if device is connected to a host
         *
         * @return true if BLE connection is active
         */
        bool isConnected() const;

        /**
         * Check if currently advertising
         *
         * @return true if advertising and waiting for connection
         */
        bool isAdvertising() const;

        /**
         * Get current Bluetooth state
         *
         * @return Current state in lifecycle
         */
        BluetoothState getState() const { return currentState; }

        /**
         * Get Bluetooth device name
         *
         * @return Device name being advertised
         */
        String getDeviceName() const { return deviceName; }

        /**
         * Check if Bluetooth is initialized
         *
         * @return true if init() was called successfully
         */
        bool isInitialized() const { return initialized; }

        // ========================================================================
        // APPLICATION LAYER INTERFACE
        // ========================================================================

        /**
         * Get BleKeyboard instance for application layer
         * Use this to send keys, media commands, etc.
         *
         * Example:
         *   if (bt.isConnected()) {
         *       bt.getBleKeyboard()->write(KEY_MEDIA_VOLUME_UP);
         *       bt.getBleKeyboard()->press(KEY_LEFT_CTRL);
         *   }
         *
         * @return Pointer to BleKeyboard instance (nullptr if not initialized)
         */
        BleKeyboard *getBleKeyboard() { return bleKeyboard; }

        /**
         * Get BleKeyboard instance (const version)
         *
         * @return Const pointer to BleKeyboard instance (nullptr if not initialized)
         */
        const BleKeyboard *getBleKeyboard() const { return bleKeyboard; }

    private:
        // BLE HID keyboard instance (application layer interface)
        BleKeyboard *bleKeyboard = nullptr;

        // State management
        BluetoothState currentState = BluetoothState::IDLE;
        bool initialized = false;

        // Device identification
        String deviceName;
        String manufacturer = String(DEVICE_MANUFACTURER);

        /**
         * Update internal state
         *
         * @param newState Target state to transition to
         */
        void setState(BluetoothState newState);

        /**
         * Generate device name using MAC address
         * Format: "CloudMouse-XXXXXXXX"
         *
         * @return Unique device name
         */
        String generateDeviceName();
    };

} // namespace CloudMouse::Network