/**
 * CloudMouse SDK - Event Bus Communication System
 * 
 * Thread-safe bidirectional communication hub for inter-task messaging using FreeRTOS queues.
 * Enables reliable event-driven architecture between Core system tasks and UI rendering tasks.
 * 
 * Architecture:
 * - Singleton pattern for global access and resource management
 * - Dual-queue system for bidirectional communication (UI ↔ Core)
 * - Non-blocking and blocking send/receive operations with configurable timeouts
 * - Queue monitoring and diagnostics for system health and debugging
 * - Thread-safe operations suitable for multi-core ESP32 architecture
 * 
 * Communication Flow:
 * Core Task → sendToUI() → UI Queue → receiveFromMain() → UI Task
 * UI Task → sendToMain() → Main Queue → receiveFromUI() → Core Task
 * 
 * Queue Management:
 * - Each queue holds up to 10 events (configurable QUEUE_SIZE)
 * - Events are copied into queue (no pointer sharing for thread safety)
 * - FIFO ordering ensures event sequence preservation
 * - Automatic queue overflow detection and reporting
 * 
 * Usage Patterns:
 * 1. Hardware → Core: Encoder events, system state changes
 * 2. Core → UI: Display updates, status changes, error notifications  
 * 3. UI → Core: User actions, configuration changes, requests
 * 4. System-wide: Boot sequence coordination, shutdown procedures
 * 
 * Performance Characteristics:
 * - Low latency: < 1ms for queue operations on ESP32
 * - Memory efficient: Fixed queue size prevents heap fragmentation
 * - Deterministic: Bounded execution time for real-time constraints
 * - Scalable: Independent queues prevent cross-task blocking
 * 
 * Error Handling:
 * - Queue full conditions are detected and reported
 * - Failed sends return false for application error handling
 * - Timeout mechanisms prevent indefinite blocking
 * - Queue state monitoring for system diagnostics
 * 
 * Thread Safety:
 * - All operations are atomic at FreeRTOS queue level
 * - No shared mutable state between tasks
 * - Safe for concurrent access from multiple cores
 * - Events are value-copied to eliminate pointer races
 */

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Events.h"

namespace CloudMouse {

/**
 * EventBus - Centralized Event Communication Hub
 * 
 * Manages thread-safe event distribution between system tasks using FreeRTOS queues.
 * Implements singleton pattern to ensure single communication channel and resource sharing.
 * 
 * Design Principles:
 * - Singleton for global accessibility and resource management
 * - Separate queues for each direction to prevent deadlocks
 * - Copy semantics for events to ensure thread safety
 * - Configurable timeouts for responsive vs. reliable communication
 * - Comprehensive monitoring for system health and debugging
 * 
 * Memory Usage:
 * - 2 queues × 10 events × ~264 bytes = ~5.3KB RAM
 * - Fixed allocation prevents heap fragmentation
 * - Predictable memory footprint for system planning
 */
class EventBus {
public:
    /**
     * Get singleton EventBus instance
     * Thread-safe lazy initialization with automatic cleanup
     * 
     * @return Reference to global EventBus instance
     */
    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }
    
    /**
     * Initialize EventBus queues and prepare for communication
     * Creates FreeRTOS queues and validates successful allocation
     * Must be called once during system initialization before any event operations
     * 
     * @note Idempotent - safe to call multiple times
     * @note Not thread-safe - call only from main initialization thread
     */
    void initialize();
    
    // ========================================================================
    // CORE-TO-UI COMMUNICATION (Main → UI Task)
    // ========================================================================
    
    /**
     * Send event from Core task to UI task
     * Queues event for UI task processing with optional timeout
     * 
     * @param event Event to send (copied into queue)
     * @param timeout Maximum wait time if queue is full (FreeRTOS ticks)
     *                0 = non-blocking (default)
     *                portMAX_DELAY = block indefinitely
     *                pdMS_TO_TICKS(ms) = wait up to specified milliseconds
     * @return true if event queued successfully, false if queue full or timeout
     * 
     * Usage Examples:
     * - sendToUI(Event(EventType::DISPLAY_UPDATE)) // Non-blocking
     * - sendToUI(event, pdMS_TO_TICKS(100)) // Wait up to 100ms
     * - sendToUI(event, portMAX_DELAY) // Block until space available
     */
    bool sendToUI(const Event& event, TickType_t timeout = 0);
    
    /**
     * Receive event from Core task in UI task
     * Retrieves next pending event from Core→UI queue
     * 
     * @param event Reference to store received event data
     * @param timeout Maximum wait time if queue is empty (FreeRTOS ticks)
     *                0 = non-blocking (default)
     *                portMAX_DELAY = block until event available
     *                pdMS_TO_TICKS(ms) = wait up to specified milliseconds
     * @return true if event received successfully, false if queue empty or timeout
     * 
     * Usage Examples:
     * - receiveFromMain(event) // Check for event, don't wait
     * - receiveFromMain(event, pdMS_TO_TICKS(50)) // Wait up to 50ms
     * - receiveFromMain(event, portMAX_DELAY) // Wait until event arrives
     */
    bool receiveFromMain(Event& event, TickType_t timeout = 0);
    
    // ========================================================================
    // UI-TO-CORE COMMUNICATION (UI → Main Task)
    // ========================================================================
    
    /**
     * Send event from UI task to Core task
     * Queues event for Core task processing with optional timeout
     * 
     * @param event Event to send (copied into queue)
     * @param timeout Maximum wait time if queue is full (FreeRTOS ticks)
     * @return true if event queued successfully, false if queue full or timeout
     * 
     * Common Use Cases:
     * - User input events (encoder clicks, button presses)
     * - Configuration changes from UI
     * - Requests for data updates or system actions
     */
    bool sendToMain(const Event& event, TickType_t timeout = 0);
    
    /**
     * Receive event from UI task in Core task
     * Retrieves next pending event from UI→Core queue
     * 
     * @param event Reference to store received event data
     * @param timeout Maximum wait time if queue is empty (FreeRTOS ticks)
     * @return true if event received successfully, false if queue empty or timeout
     * 
     * Common Use Cases:
     * - Processing user input in main application logic
     * - Handling configuration updates from UI
     * - Responding to UI requests for data or actions
     */
    bool receiveFromUI(Event& event, TickType_t timeout = 0);
    
    // ========================================================================
    // QUEUE MONITORING AND DIAGNOSTICS
    // ========================================================================
    
    /**
     * Get number of pending events in UI queue (Core→UI)
     * Useful for monitoring queue depth and system load
     * 
     * @return Number of events waiting to be processed by UI task
     */
    uint32_t getUIQueueCount() const;
    
    /**
     * Get number of pending events in Main queue (UI→Core)
     * Useful for monitoring queue depth and system load
     * 
     * @return Number of events waiting to be processed by Core task
     */
    uint32_t getMainQueueCount() const;
    
    /**
     * Check if EventBus is properly initialized
     * Validates that both queues are created and operational
     * 
     * @return true if EventBus is ready for event operations
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * Get maximum queue capacity
     * Returns configured queue size for capacity planning
     * 
     * @return Maximum number of events each queue can hold
     */
    uint32_t getQueueCapacity() const { return QUEUE_SIZE; }
    
    /**
     * Check if UI queue is full
     * Useful for implementing backpressure or alternative handling
     * 
     * @return true if UI queue cannot accept more events
     */
    bool isUIQueueFull() const { return getUIQueueCount() >= QUEUE_SIZE; }
    
    /**
     * Check if Main queue is full
     * Useful for implementing backpressure or alternative handling
     * 
     * @return true if Main queue cannot accept more events
     */
    bool isMainQueueFull() const { return getMainQueueCount() >= QUEUE_SIZE; }
    
    /**
     * Log current queue status to Serial
     * Debugging utility for system health monitoring
     * Output format: "[EventBus] UI Queue: X, Main Queue: Y"
     */
    void logStatus() const;
    
    /**
     * Get detailed queue statistics
     * Advanced diagnostics for performance analysis
     * 
     * @param uiCount Output parameter for UI queue depth
     * @param mainCount Output parameter for Main queue depth
     * @param uiFull Output parameter for UI queue full status
     * @param mainFull Output parameter for Main queue full status
     */
    void getQueueStats(uint32_t& uiCount, uint32_t& mainCount, bool& uiFull, bool& mainFull) const {
        uiCount = getUIQueueCount();
        mainCount = getMainQueueCount();
        uiFull = isUIQueueFull();
        mainFull = isMainQueueFull();
    }
    
private:
    // FreeRTOS queue handles for bidirectional communication
    QueueHandle_t uiToMainQueue = nullptr;    // UI task → Core task communication
    QueueHandle_t mainToUIQueue = nullptr;    // Core task → UI task communication
    
    // Configuration constants
    static const uint32_t QUEUE_SIZE = 10;   // Maximum events per queue
    
    // Initialization state
    bool initialized = false;                 // Tracks successful initialization
    
    /**
     * Private constructor for singleton pattern
     * Prevents direct instantiation - use instance() method
     */
    EventBus() = default;
    
    /**
     * Deleted copy constructor and assignment operator
     * Prevents singleton duplication
     */
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
};

} // namespace CloudMouse

#endif