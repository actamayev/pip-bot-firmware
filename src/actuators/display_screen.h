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
        void showStartScreen();

        // Display object
        Adafruit_SSD1306 display;
        
        // State flags
        bool initialized = false;
        bool customScreenActive = false;
        
        // Timing management
        unsigned long lastUpdateTime = 0;

        // Constants
        static const unsigned long UPDATE_INTERVAL = 50;          // 50ms (20fps)
        bool isShowingStartScreen = false;
};
