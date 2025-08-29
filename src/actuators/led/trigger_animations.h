#pragma once

#include <Adafruit_NeoPixel.h>
#include "networking/protocol.h"
#include "utils/config.h"
#include "utils/structs.h"

class TriggerAnimations {
    public:
        TriggerAnimations(Adafruit_NeoPixel& strip);
        
        void startS2P1Sequence();
        void stopS2P1Sequence();
        void startS2P4LightShow();
        void stopS2P4LightShow();
        void update();
        
        bool isS2P1Active() const { return s2p1Active; }
        bool isS2P4Active() const { return s2p4Active; }
        
        // LED sequence order (back_right → middle_right → etc.)
        static constexpr uint8_t s2p1LedSequence[6] = {7, 0, 1, 4, 5, 6}; // Skip headlights (2,3)
        
        // Color progression (Red → Orange → Yellow → Green → Blue → Purple)
        static constexpr uint8_t s2p1ColorSequence[6][3] = {
            {255, 0, 0},     // Red
            {255, 165, 0},   // Orange  
            {255, 255, 0},   // Yellow
            {0, 255, 0},     // Green
            {0, 0, 255},     // Blue
            {128, 0, 128}    // Purple
        };
        
        // Timing constants
        static constexpr unsigned long S2P1_FADE_TIME = 1000; // 1000ms total fade time
        static constexpr unsigned long S2P1_UPDATE_INTERVAL = 20; // 20ms updates for smooth fading
        static constexpr unsigned long S2P4_DURATION = 5000; // 5 seconds
        static constexpr unsigned long S2P4_UPDATE_INTERVAL = 50; // 50ms updates
        
    private:
        Adafruit_NeoPixel& strip;
        
        // S2_P1 sequence state
        bool s2p1Active = false;
        uint8_t currentLedIndex = 0;
        uint8_t currentColorIndex = 0;
        unsigned long lastS2P1Update = 0;
        
        // Fade state
        bool isFadingOut = false;
        uint8_t currentBrightness = 0;
        uint8_t targetBrightness = 255;
        
        // S2_P4 light show state
        bool s2p4Active = false;
        unsigned long s2p4StartTime = 0;
        unsigned long lastS2P4Update = 0;
        uint8_t s2p4Step = 0;
        
        void updateS2P1Sequence();
        void updateS2P4LightShow();
};

extern TriggerAnimations triggerAnimations;