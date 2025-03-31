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
    private:
        void set_led_to_color(uint8_t red, uint8_t green, uint8_t blue);  
};

extern RgbLed rgbLed;
