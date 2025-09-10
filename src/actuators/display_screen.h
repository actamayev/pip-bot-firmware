#pragma once
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "networking/serial_queue_manager.h"
#include "utils/preferences_manager.h"

#define DISPLAY_BUFFER_SIZE ((SCREEN_WIDTH * SCREEN_HEIGHT) / 8)

class DisplayScreen: public Singleton<DisplayScreen> {
    friend class Singleton<DisplayScreen>;  // Allow Singleton to access private constructor
    friend class CareerQuestTriggers;  // Allow Singleton to access private constructor
    friend class DinoRunner;  // Allow Singleton to access private constructor

    public:
        bool init(bool showStartup);
        
        // Main update method to call in the task loop
        void update();

        void showCustomBuffer(const uint8_t* buffer);

        void showLowBatteryScreen();

        void turnDisplayOff();
        void turnDisplayOn();
        
        // Performance tracking
        unsigned long getDisplayUpdateCount() const { return displayUpdates; }
        unsigned long getContentGenerationCount() const { return contentGenerations; }
        unsigned long getSkippedUpdateCount() const { return skippedUpdates; }
        float getDisplayUpdateRate() const;
        void resetPerformanceCounters();

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
        bool hasContentChanged();
        void copyCurrentBuffer();
        void generateContentToBuffer();
};
