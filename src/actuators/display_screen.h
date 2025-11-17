#pragma once
#include <Arduino.h>

#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "custom_interpreter/bytecode_vm.h"
#include "networking/serial_queue_manager.h"
#include "sensors/sensor_data_buffer.h"
#include "utils/config.h"
#include "utils/preferences_manager.h"
#include "utils/singleton.h"
#include "utils/structs.h"

#define DISPLAY_BUFFER_SIZE ((SCREEN_WIDTH * SCREEN_HEIGHT) / 8)

class DisplayScreen : public Singleton<DisplayScreen> {
    friend class Singleton<DisplayScreen>; // Allow Singleton to access private constructor
    friend class CareerQuestTriggers;      // Allow Singleton to access private constructor
    friend class DinoRunner;               // Allow Singleton to access private constructor

  public:
    static bool init(bool show_startup);

    // Main update method to call in the task loop
    static void update();

    static void show_custom_buffer(const uint8_t* buffer);

    static void show_low_battery_screen();

    static void turn_display_off();
    static void turn_display_on();

    // Performance tracking
    uint32_t get_display_update_count() const {
        return _displayUpdates;
    }
    uint32_t get_content_generation_count() const {
        return _contentGenerations;
    }
    uint32_t get_skipped_update_count() const {
        return _skippedUpdates;
    }
    static float get_display_update_rate();
    void reset_performance_counters();

    // Screen display methods
    static void show_start_screen();

  private:
    // Private constructor for singleton
    DisplayScreen() = default;

    // Helper method
    static void render_display();

    // Drawing utilities
    void clear();
    static void draw_text(const String& text, uint16_t x, uint16_t y, uint16_t size = 1);
    static void draw_centered_text(const String& text, uint16_t y, uint16_t size = 1);

    // Display object
    Adafruit_SSD1306 _display;

    // State flags
    bool _initialized = false;
    bool _customScreenActive = false;
    bool _displayOff = false;

    // Timing management
    uint32_t _lastUpdateTime = 0;
    uint32_t _lastContentGeneration = 0;
    uint32_t _lastDisplayUpdate = 0;

    // Buffers for change detection
    uint8_t _stagingBuffer[DISPLAY_BUFFER_SIZE]{};
    uint8_t _currentDisplayBuffer[DISPLAY_BUFFER_SIZE]{};

    // Performance tracking
    uint32_t _displayUpdates = 0;
    uint32_t _contentGenerations = 0;
    uint32_t _skippedUpdates = 0;
    uint32_t _perfStartTime = 0;

    // Constants
    static const uint32_t UPDATE_INTERVAL = 50; // 50ms (20fps)
    bool _isShowingStartScreen = false;

    // Private methods for buffer management
    static bool has_content_changed();
    static void copy_current_buffer();
    static void generate_content_to_buffer();
};
