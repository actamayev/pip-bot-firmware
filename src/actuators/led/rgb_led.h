#pragma once

#include <Adafruit_NeoPixel.h>
#include "./led_animations.h"
#include "../../utils/config.h"
#include "../../utils/structs.h"

class RgbLed {
    public:
        // Basic color controls
        void turn_led_off();
        void set_led_red();
        void set_led_green();
        void set_led_blue();
        void set_led_white();
        void set_led_purple();
        void set_led_yellow();

        // Individual LED controls
        void set_top_left_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_top_right_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_back_left_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_back_right_led(uint8_t red, uint8_t green, uint8_t blue);
        void set_all_leds_to_color(uint8_t red, uint8_t green, uint8_t blue);  
        
        // Get current color values
        uint8_t getCurrentRed() const { return currentRed; }
        uint8_t getCurrentGreen() const { return currentGreen; }
        uint8_t getCurrentBlue() const { return currentBlue; }
        
        // Default colors for each LED
        uint8_t defaultColors[6][3] = {
            {0, 0, 0}, // top_left    - R,G,B
            {0, 0, 0}, // top_right   - R,G,B
            {0, 0, 0}, // middle_left - R,G,B
            {0, 0, 0}, // middle_right- R,G,B
            {0, 0, 0}, // back_left   - R,G,B
            {0, 0, 0}  // back_right  - R,G,B
        };
        bool defaultColorsSet[6] = {false, false, false, false, false, false};

        void captureCurrentState();
        void restoreCapturedState();

    private:
        bool processLedUpdate(int ledIndex, uint8_t red, uint8_t green, uint8_t blue);
        
        // Current LED state
        uint8_t currentRed = 0;
        uint8_t currentGreen = 0;
        uint8_t currentBlue = 0;
        
        struct LedState {
            uint8_t colors[6][3];  // Colors for all 6 LEDs
            LedTypes::AnimationType animation;
            int animationSpeed;
            bool wasAnimationActive;
        };

        LedState capturedState;
};

extern RgbLed rgbLed;