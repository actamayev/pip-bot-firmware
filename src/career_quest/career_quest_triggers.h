#pragma once

#include <Adafruit_NeoPixel.h>
#include "networking/protocol.h"
#include "utils/config.h"
#include "utils/structs.h"
#include "actuators/display_screen.h"
#include "actuators/led/rgb_led.h"
#include "actuators/led/led_animations.h"
#include "actuators/buttons.h"
#include "actuators/speaker.h"

class CareerQuestTriggers {
    public:
        CareerQuestTriggers(Adafruit_NeoPixel& strip);

        void startS2P1Sequence();
        void stopS2P1Sequence();
        void startS2P4LightShow();
        void stopS2P4LightShow();
        void startS3P3DisplayDemo();
        void stopS3P3DisplayDemo();
        void startS7P4ButtonDemo();
        void stopS7P4ButtonDemo();
        void startS5P4LedVisualization();
        void stopS5P4LedVisualization();
        void stopAllCareerQuestTriggers();
        void update();
        
        bool isS2P1Active() const { return s2p1Active; }
        bool isS2P4Active() const { return s2p4Active; }
        bool isS3P3Active() const { return s3p3Active; }
        bool isS7P4Active() const { return s7p4Active; }
        bool isS5P4Active() const { return s5p4Active; }
        
        void renderS3P3Animation();
        
        // LED sequence order (back_right → middle_right → etc.) including headlights
        static constexpr uint8_t s2p1LedSequence[8] = {7, 0, 1, 2, 3, 4, 5, 6}; // Include headlights (2,3)
        
        // Color progression (Red → Orange → Yellow → Green → Blue → Purple → Cyan → Magenta)
        static constexpr uint8_t s2p1ColorSequence[8][3] = {
            {255, 0, 0},     // Red
            {255, 127, 0},   // Orange
            {255, 255, 0},   // Yellow
            {127, 255, 0},   // Chartreuse
            {0, 255, 0},     // Green
            {0, 255, 255},   // Cyan
            {0, 0, 255},     // Blue
            {128, 0, 255}    // Purple
        };
        
        // Timing constants
        static constexpr unsigned long S2P1_FADE_TIME = 1000; // 1000ms total fade time
        static constexpr unsigned long S2P1_UPDATE_INTERVAL = 10; // 10ms updates for faster, smoother fading
        static constexpr unsigned long S2P4_DURATION = 5000; // 5 seconds
        static constexpr unsigned long S2P4_UPDATE_INTERVAL = 1000; // 50ms updates
        static constexpr unsigned long S3P3_UPDATE_INTERVAL = 50; // 50ms updates for smooth scrolling
        
    private:
        Adafruit_NeoPixel& strip;
        
        // S2_P1 sequence state
        bool s2p1Active = false;
        uint8_t currentLedIndex = 0;
        uint8_t currentColorIndex = 0;
        unsigned long lastS2P1Update = 0;
        
        // Fade state
        bool isFadingOut = false;
        bool isExitFading = false;
        uint8_t currentBrightness = 0;
        uint8_t targetBrightness = 255;
        
        // S2_P4 light show state
        bool s2p4Active = false;
        unsigned long s2p4StartTime = 0;
        unsigned long lastS2P4Update = 0;
        uint8_t s2p4Step = 0;
        bool s2p4ExitFading = false;
        uint8_t s2p4CurrentBrightness = 255;
        
        // S3_P3 display demo state
        bool s3p3Active = false;
        unsigned long lastS3P3Update = 0;
        unsigned long s3p3StartTime = 0;
        uint8_t s3p3AnimationStep = 0;
        
        // S7_P4 button demo state
        bool s7p4Active = false;
        bool s7p4ExitFading = false;
        uint8_t s7p4CurrentBrightness = 255;
        unsigned long lastS7P4Update = 0;
        
        // S5_P4 IMU LED visualization state
        bool s5p4Active = false;
        bool s5p4ExitFading = false;
        unsigned long lastS5P4Update = 0;
        static constexpr unsigned long S5P4_UPDATE_INTERVAL = 20; // 20ms for responsive visualization
        
        void updateS2P1Sequence();
        void updateS2P4LightShow();
        void updateS7P4ButtonDemo();
        void updateS5P4LedVisualization();
};

extern CareerQuestTriggers careerQuestTriggers;
