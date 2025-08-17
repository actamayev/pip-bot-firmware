#pragma once
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "networking/serial_queue_manager.h"

class DisplayScreen: public Singleton<DisplayScreen> {
    friend class Singleton<DisplayScreen>;  // Allow Singleton to access private constructor

    public:
        bool init();
        
        // Main update method to call in the task loop
        void update();

        void showDistanceSensors(SideTofCounts sideTofCounts);
        void showCustomBuffer(const uint8_t* buffer);

        void turnDisplayOff();
    private:
        // Private constructor for singleton
        DisplayScreen() = default;
        
        // Helper method
        void renderDisplay();

        // Drawing utilities
        void clear();
        void drawText(const String& text, uint16_t x, uint16_t y, uint16_t size = 1);
        void drawCenteredText(const String& text, uint16_t y, uint16_t size = 1);

        // Screen display methods
        void showStartScreen(bool resetTimer);

        // Display object
        Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
        
        // State flags
        bool initialized = false;
        bool isShowingStartScreen = false;
        bool customScreenActive = false;
        bool redrawStartScreen = false;
        
        // Timing management
        unsigned long lastUpdateTime = 0;
        unsigned long startScreenStartTime = 0;

        // Constants
        static const unsigned long START_SCREEN_DURATION = 2000;  // 2 seconds
        static const unsigned long UPDATE_INTERVAL = 50;          // 50ms (20fps)
};
