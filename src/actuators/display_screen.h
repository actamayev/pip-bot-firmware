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
    bool init(bool showStartup);

    // Main update method to call in the task loop
    void update();

    void show_custom_buffer(const uint8_t* buffer);

    void show_low_battery_screen();

    void turn_display_off();
    void turn_display_on();

    // Performance tracking
    uint32_t get_display_update_count() const {
        return displayUpdates;
    }
    uint32_t get_content_generation_count() const {
        return contentGenerations;
    }
    uint32_t get_skipped_update_count() const {
        return skippedUpdates;
    }
    float get_display_update_rate() const;
    void reset_performance_counters();

    // Screen display methods
    void show_start_screen();

  private:
    // Private constructor for singleton
    DisplayScreen() = default;

    // Helper method
    void render_display();

    // Drawing utilities
    void clear();
    void draw_text(const String& text, uint16_t x, uint16_t y, uint16_t size = 1);
    void draw_centered_text(const String& text, uint16_t y, uint16_t size = 1);

    // Display object
    Adafruit_SSD1306 display;

    // State flags
    bool initialized = false;
    bool customScreenActive = false;
    bool displayOff = false;

    // Timing management
    uint32_t lastUpdateTime = 0;
    uint32_t lastContentGeneration = 0;
    uint32_t lastDisplayUpdate = 0;

    // Buffers for change detection
    uint8_t stagingBuffer[DISPLAY_BUFFER_SIZE];
    uint8_t currentDisplayBuffer[DISPLAY_BUFFER_SIZE];

    // Performance tracking
    uint32_t displayUpdates = 0;
    uint32_t contentGenerations = 0;
    uint32_t skippedUpdates = 0;
    uint32_t perfStartTime = 0;

    // Constants
    static const uint32_t UPDATE_INTERVAL = 50; // 50ms (20fps)
    bool isShowingStartScreen = false;

    // Private methods for buffer management
    bool has_content_changed();
    void copy_current_buffer();
    void generate_content_to_buffer();
};
