#pragma once

#include <Adafruit_NeoPixel.h>
#include "../../utils/config.h"

class RgbLed {
    public:
        void turn_led_off();
        void set_led_red();
        void set_led_green();
        void set_led_blue();
        void set_led_white();
        void set_led_purple();
        void update();
        void startBreathing(int speed = 2000);
        void stopBreathing();

        void pauseBreathing();
        void fadeOut();

        void set_top_left_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_top_right_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_back_left_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_back_right_led(uint8_t red, uint8_t green, uint8_t blue);
        void startStrobe(uint8_t red, uint8_t green, uint8_t blue, int speed);
        void startRainbow(int cyecleTime = 2000);
        void stopAllAnimations();

        uint8_t defaultColors[6][3] = {
            {0, 0, 0}, // top_left    - R,G,B
            {0, 0, 0}, // top_right   - R,G,B
            {0, 0, 0}, // middle_left - R,G,B
            {0, 0, 0}, // middle_right- R,G,B
            {0, 0, 0}, // back_left   - R,G,B
            {0, 0, 0}  // back_right  - R,G,B
        };
        bool defaultColorsSet[6] = {false, false, false, false, false, false};

    private:
        void set_all_leds_to_color(uint8_t red, uint8_t green, uint8_t blue);  
        bool isBreathing = false;
        unsigned long lastBreathUpdate = 0;
        uint8_t breathMin[3] = {0, 0, 0}; // Minimum RGB values
        uint8_t breathMax[3] = {0, 0, 0}; // Maximum RGB values
        float breathProgress = 0.0; // 0.0 to 1.0 to track position in the cycle
        int breatheSpeed = 0; // milliseconds between brightness changes
        bool breatheDirection = true; // true = increasing, false = decreasing

        uint8_t currentRed = 0;
        uint8_t currentGreen = 0;
        uint8_t currentBlue = 0;

        uint8_t getCurrentRed() { return currentRed; }
        uint8_t getCurrentGreen() { return currentGreen; }
        uint8_t getCurrentBlue() { return currentBlue; }

        bool isFadingOut = false;  // Add this new flag

        bool isStrobing = false;
        bool isRainbow = false;
        unsigned long lastStrobeUpdate = 0;
        unsigned long lastRainbowUpdate = 0;
        int strobeSpeed = 0;
        int rainbowCycleTime = 2000; // Total cycle time in ms (default 2s)
        unsigned long rainbowStepTime = 0; // Time per step, calculated based on cycle time
        
        uint8_t strobeColor[3] = {0, 0, 0};
        bool strobeState = false;
        int rainbowHue = 0;

        uint32_t colorHSV(uint8_t h, uint8_t s = 255, uint8_t v = 255);
        void updateBreathingColor();
        void updateStrobeColor();
        bool processLedUpdate(int ledIndex, uint8_t red, uint8_t green, uint8_t blue);
    };

extern RgbLed rgbLed;
