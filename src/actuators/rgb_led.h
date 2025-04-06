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
        void startBreathing(uint8_t redMin, uint8_t redMax, 
            uint8_t greenMin, uint8_t greenMax, 
            uint8_t blueMin, uint8_t blueMax, 
            int speed);
        void stopBreathing();

    private:
        void set_led_to_color(uint8_t red, uint8_t green, uint8_t blue);  
        uint8_t breathMin[3] = {0, 0, 0}; // Minimum RGB values
        uint8_t breathMax[3] = {0, 0, 0}; // Maximum RGB values
        float breathProgress = 0.0; // 0.0 to 1.0 to track position in the cycle
};

extern RgbLed rgbLed;
