#pragma once
#include <Wire.h>
#include "Arduino.h"
#include <Adafruit_SSD1306.h>
#include "../utils/config.h"
#include "../utils/structs.h"
#include "../utils/singleton.h"

class DisplayScreen: public Singleton<DisplayScreen> {
    friend class Singleton<DisplayScreen>;  // Allow Singleton to access private constructor

    public:
        // Explicit initialization with Wire reference
        bool init(TwoWire& wire);
        
        // Main update method to call in the task loop
        void update();
        
        // Screen display methods
        void showStartScreen(bool resetTimer = true);
        void showDistanceSensors(SideTofDistances sideTofDistances);
        void resetToStartScreen();
        
        // Drawing utilities
        void clear();
        void drawText(const String& text, int x, int y, int size = 1);
        void drawCenteredText(const String& text, int y, int size = 1);
        void drawProgressBar(int progress, int y);
        
        // Status checks
        bool isInitialized() const { return initialized; }
        bool isStartupComplete() const { return !isShowingStartScreen; }
        
        // Get access to the underlying display object if needed
        Adafruit_SSD1306* getDisplay() { return &display; }

    private:
        // Private constructor for singleton
        DisplayScreen() : 
            initialized(false),
            isShowingStartScreen(false),
            customScreenActive(false),
            redrawStartScreen(false),
            lastUpdateTime(0),
            startScreenStartTime(0) {}
        
        // Helper method
        void renderDisplay();
        
        // Display object
        Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, nullptr, OLED_RESET);
        
        // State flags
        bool initialized;
        bool isShowingStartScreen;
        bool customScreenActive;
        bool redrawStartScreen;
        
        // Timing management
        unsigned long lastUpdateTime;
        unsigned long startScreenStartTime;
        
        // Constants
        static const unsigned long START_SCREEN_DURATION = 2000;  // 2 seconds
        static const unsigned long UPDATE_INTERVAL = 50;          // 50ms (20fps)
};
