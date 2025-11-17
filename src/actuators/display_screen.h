#pragma once
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "networking/serial_queue_manager.h"
#include "utils/preferences_manager.h"
#include "custom_interpreter/bytecode_vm.h"
#include "sensors/sensor_data_buffer.h"

#define DISPLAY_BUFFER_SIZE ((SCREEN_WIDTH * SCREEN_HEIGHT) / 8)

class DisplayScreen: public Singleton<DisplayScreen> {
    friend class Singleton<DisplayScreen>;  // Allow Singleton to access private constructor
    friend class CareerQuestTriggers;  // Allow Singleton to access private constructor
    friend class DinoRunner;  // Allow Singleton to access private constructor

    public:
        bool init(bool showStartup);
        
        // Main update method to call in the task loop
        void update();

        void show_custom_buffer(const uint8_t* buffer);

        void show_low_battery_screen();

        void turn_display_off();
        void turn_display_on();
        
        // Performance tracking
        unsigned long get_display_update_count() const { return displayUpdates; }
        unsigned long get_content_generation_count() const { return contentGenerations; }
        unsigned long get_skipped_update_count() const { return skippedUpdates; }
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
        unsigned long lastUpdateTime = 0;
        unsigned long lastContentGeneration = 0;
        unsigned long lastDisplayUpdate = 0;

        // Buffers for change detection
        uint8_t stagingBuffer[DISPLAY_BUFFER_SIZE];
        uint8_t currentDisplayBuffer[DISPLAY_BUFFER_SIZE];
        
        // Performance tracking
        unsigned long displayUpdates = 0;
        unsigned long contentGenerations = 0;
        unsigned long skippedUpdates = 0;
        unsigned long perfStartTime = 0;

        // Constants
        static const unsigned long UPDATE_INTERVAL = 50;          // 50ms (20fps)
        bool isShowingStartScreen = false;
        
        // Private methods for buffer management
        bool has_content_changed();
        void copy_current_buffer();
        void generate_content_to_buffer();
};
