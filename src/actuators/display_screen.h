#pragma once
#include <Wire.h>
#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../utils/config.h"
#include "../utils/singleton.h"

class DisplayScreen: public Singleton<DisplayScreen> {
    friend class Singleton<DisplayScreen>;  // Allow Singleton to access private constructor
    
    public:
        // Explicit initialization with Wire reference
        bool init(TwoWire& wire);
        
        // Main update method to handle all display logic
        void update();
        
        // Drawing functions - can be called externally if needed
        void clear();
        void drawText(const String& text, int x, int y, int size = 1);
        void drawCenteredText(const String& text, int y, int size = 1);
        void drawProgressBar(int progress, int y);
        void startStartupSequence();
        
        // Status checks
        bool isInitialized() const { return initialized; }
        bool isStartupComplete() const { return startupComplete; }
        
        // Get access to the underlying display object if needed
        Adafruit_SSD1306* getDisplay() { return &display; }

    private:
        // Private constructor for singleton - will be called by Singleton<T>::getInstance()
        DisplayScreen() : 
            initialized(false),
            startupActive(false),
            startupComplete(false),
            lastUpdateTime(0),
            startupStartTime(0),
            carPosition(-20),
            displayMode(0),
            animationCounter(0) {}
        
        // Private methods
        void updateStartupAnimation();
        void updateRegularDisplay();
        void renderDisplay();
        
        // Display object
        Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, nullptr, OLED_RESET);
        
        // State flags
        bool initialized;
        bool startupActive;
        bool startupComplete;
        
        // Timing management
        unsigned long lastUpdateTime;
        unsigned long startupStartTime;
        
        // Animation variables
        int carPosition;
        int displayMode;
        int animationCounter;
        
        // Constants
        static const unsigned long STARTUP_DURATION = 2000;  // 2 seconds
        static const unsigned long UPDATE_INTERVAL = 50;     // 50ms (20fps)
};
