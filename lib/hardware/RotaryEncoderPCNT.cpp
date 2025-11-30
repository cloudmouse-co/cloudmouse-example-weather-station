/**
 * CloudMouse SDK - Cross-Platform Rotary Encoder PCNT Implementation
 * 
 * Implementation of ESP32 PCNT hardware abstraction with automatic ESP-IDF version
 * detection and API compatibility handling for rotary encoder quadrature processing.
 */

#include "RotaryEncoderPCNT.h"
#include "../utils/Logger.h"
// ============================================================================
// HARDWARE INITIALIZATION IMPLEMENTATION
// ============================================================================

void RotaryEncoderPCNT::init() {
#ifdef USE_OLD_PCNT_API
    // ============ ESP-IDF 4.4 API (PlatformIO) ============
    
    SDK_LOGGER("🔧 Initializing PCNT on pins A=%d, B=%d\n", pin_a, pin_b);
    
    // Configure GPIO pins with pull-up resistors
    pinMode(pin_a, INPUT_PULLUP);
    pinMode(pin_b, INPUT_PULLUP);

    // Configure PCNT unit for channel 0 (pin A)
    pcnt_config_t pcnt_config_a;
    pcnt_config_a.pulse_gpio_num = pin_a;
    pcnt_config_a.ctrl_gpio_num = pin_b;
    pcnt_config_a.lctrl_mode = PCNT_MODE_KEEP;
    pcnt_config_a.hctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config_a.pos_mode = PCNT_COUNT_INC;
    pcnt_config_a.neg_mode = PCNT_COUNT_DEC;
    pcnt_config_a.counter_h_lim = 32767;
    pcnt_config_a.counter_l_lim = -32768;
    pcnt_config_a.unit = PCNT_UNIT_0;
    pcnt_config_a.channel = PCNT_CHANNEL_0;
    
    esp_err_t err = pcnt_unit_config(&pcnt_config_a);
    if (err != ESP_OK) {
        SDK_LOGGER("❌ PCNT channel 0 config failed: %d\n", err);
        return;
    }
    
    // Configure PCNT unit for channel 1 (pin B)
    pcnt_config_t pcnt_config_b;
    pcnt_config_b.pulse_gpio_num = pin_b;
    pcnt_config_b.ctrl_gpio_num = pin_a;
    pcnt_config_b.lctrl_mode = PCNT_MODE_KEEP;
    pcnt_config_b.hctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config_b.pos_mode = PCNT_COUNT_DEC;
    pcnt_config_b.neg_mode = PCNT_COUNT_INC;
    pcnt_config_b.counter_h_lim = 32767;
    pcnt_config_b.counter_l_lim = -32768;
    pcnt_config_b.unit = PCNT_UNIT_0;
    pcnt_config_b.channel = PCNT_CHANNEL_1;
    
    err = pcnt_unit_config(&pcnt_config_b);
    if (err != ESP_OK) {
        SDK_LOGGER("❌ PCNT channel 1 config failed: %d\n", err);
        return;
    }
    
    // Set glitch filter (if specified)
    if (glitch_time > 0) {
        err = pcnt_set_filter_value(PCNT_UNIT_0, glitch_time);
        if (err == ESP_OK) {
            pcnt_filter_enable(PCNT_UNIT_0);
            SDK_LOGGER("✅ Glitch filter enabled: %dns\n", glitch_time);
        }
    }
    
    // Initialize counter
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
    
    SDK_LOGGER("✅ RotaryEncoder initialized (IDF 4.4 API)");
    
#elif defined(USE_NEW_PCNT_API)
    // ============ ESP-IDF 5.x API (Arduino IDE) ============
    
    SDK_LOGGER("🔧 Initializing PCNT on pins A=%d, B=%d\n", pin_a, pin_b);
    
    // Configure PCNT unit
    pcnt_unit_config_t unit_config = {
        .low_limit = low_limit,
        .high_limit = high_limit,
    };
    pcnt_new_unit(&unit_config, &unit);
    
    // Configure glitch filter
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = glitch_time,
    };
    pcnt_unit_set_glitch_filter(unit, &filter_config);
    
    // Configure channel A
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = pin_a,
        .level_gpio_num = pin_b,
    };
    pcnt_new_channel(unit, &chan_a_config, &chan_a);
    
    // Configure channel B
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = pin_b,
        .level_gpio_num = pin_a,
    };
    pcnt_new_channel(unit, &chan_b_config, &chan_b);
    
    // Set channel actions
    pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    
    pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    
    // Enable and start unit
    pcnt_unit_enable(unit);
    pcnt_unit_clear_count(unit);
    pcnt_unit_start(unit);
    
    SDK_LOGGER("✅ RotaryEncoder initialized (IDF 5.x API)");
#endif
}

// ============================================================================
// HARDWARE DEINITIALIZATION IMPLEMENTATION
// ============================================================================

void RotaryEncoderPCNT::deinit() {
#ifdef USE_OLD_PCNT_API
    // ESP-IDF 4.4 cleanup
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    SDK_LOGGER("🔧 PCNT deinitialized (IDF 4.4 API)");
    
#elif defined(USE_NEW_PCNT_API)
    // ESP-IDF 5.x cleanup
    if (unit) {
        pcnt_unit_stop(unit);
        pcnt_unit_disable(unit);
        pcnt_del_channel(chan_a);
        pcnt_del_channel(chan_b);
        pcnt_del_unit(unit);
        unit = nullptr;
        chan_a = nullptr;
        chan_b = nullptr;
        SDK_LOGGER("🔧 PCNT deinitialized (IDF 5.x API)");
    }
#endif
}

// ============================================================================
// POSITION READING IMPLEMENTATION
// ============================================================================

int RotaryEncoderPCNT::position() {
    int value = 0;
    
#ifdef USE_OLD_PCNT_API
    // ESP-IDF 4.4 position reading
    int16_t count = 0;
    esp_err_t err = pcnt_get_counter_value(PCNT_UNIT_0, &count);
    if (err == ESP_OK) {
        value = count + offset;
    } else {
        // Error handling: return last valid value
        static int last_valid = 0;
        value = last_valid;
        SDK_LOGGER("⚠️ PCNT read error: %d, using last valid: %d\n", err, last_valid);
    }
    
#elif defined(USE_NEW_PCNT_API)
    // ESP-IDF 5.x position reading
    int temp_count = 0;
    if (unit) {
        esp_err_t err = pcnt_unit_get_count(unit, &temp_count);
        if (err == ESP_OK) {
            value = temp_count + offset;
        } else {
            SDK_LOGGER("⚠️ PCNT read error: %d\n", err);
            value = offset; // Fallback to offset only
        }
    } else {
        SDK_LOGGER("⚠️ PCNT unit not initialized");
        value = offset;
    }
#endif
    
    return value;
}

// ============================================================================
// POSITION CONTROL IMPLEMENTATION
// ============================================================================

void RotaryEncoderPCNT::setPosition(int pos) {
    // Update offset to new position
    offset = pos;
    
    // Clear hardware counter to establish new zero point
#ifdef USE_OLD_PCNT_API
    pcnt_counter_clear(PCNT_UNIT_0);
    SDK_LOGGER("🔧 Position set to %d (IDF 4.4)\n", pos);
    
#elif defined(USE_NEW_PCNT_API)
    if (unit) {
        pcnt_unit_clear_count(unit);
        SDK_LOGGER("🔧 Position set to %d (IDF 5.x)\n", pos);
    } else {
        SDK_LOGGER("⚠️ Cannot set position - PCNT unit not initialized");
    }
#endif
}

void RotaryEncoderPCNT::zero() {
    // Reset to default starting position
    setPosition(START_POS_DEFAULT);
}