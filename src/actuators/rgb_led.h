#pragma once

#include <Adafruit_NeoPixel.h>
#include "../utils/config.h"

class RgbLed {
    public:
        void turn_led_off();
        void set_led_red();
        void set_led_green();
        void set_led_blue();
        void set_led_white();
        void set_led_purple();
        bool isBreathing = false;
        unsigned long lastBreathUpdate = 0;
        uint8_t breathTarget[3] = {0, 0, 0}; // Target RGB color
        uint8_t currentBrightness = 0;
        int breatheSpeed = 0; // milliseconds between brightness changes
        bool breatheDirection = true; // true = increasing, false = decreasing
        void update();
        void startBreathing(
            uint8_t redMin, uint8_t redMax, 
            uint8_t greenMin, uint8_t greenMax, 
            uint8_t blueMin, uint8_t blueMax, 
            int speed
        );
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
        void startRainbow(int speed);
        void stopAllAnimations();
        void set_all_leds_to_color(uint8_t red, uint8_t green, uint8_t blue);  

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
        uint8_t breathMin[3] = {0, 0, 0}; // Minimum RGB values
        uint8_t breathMax[3] = {0, 0, 0}; // Maximum RGB values
        float breathProgress = 0.0; // 0.0 to 1.0 to track position in the cycle

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
        int rainbowSpeed = 0;
        uint8_t strobeColor[3] = {0, 0, 0};
        bool strobeState = false;
        int rainbowHue = 0;

        uint32_t colorHSV(uint8_t h, uint8_t s = 255, uint8_t v = 255);
    };

extern RgbLed rgbLed;
