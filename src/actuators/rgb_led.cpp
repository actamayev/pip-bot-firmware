#include "./rgb_led.h"
#include "../utils/config.h"

RgbLed rgbLed;

// Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, ESP_LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS1, ESP_LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS2, ESP_LED_PIN2, NEO_GRB + NEO_KHZ800);

void RgbLed::turn_led_off() {
    set_led_to_color(0, 0, 0);
}

void RgbLed::set_led_red() {
    set_led_to_color(255, 0, 0);
}

void RgbLed::set_led_green() {
    set_led_to_color(0, 255, 0);
}

void RgbLed::set_led_blue() {
    set_led_to_color(0, 0, 255);
}

void RgbLed::set_led_white() {
    set_led_to_color(255, 255, 255);
}

void RgbLed::set_led_purple() {
    set_led_to_color(255, 0, 255);
}

void RgbLed::set_led_to_color(uint8_t red, uint8_t green, uint8_t blue) {
    // strip.setPixelColor(0, strip.Color(red, green, blue));

    // Set all LEDs to the same color
    for(int i = 0; i < NUM_LEDS1; i++) {
        strip1.setPixelColor(i, strip1.Color(red, green, blue));
    }
    for(int i = 0; i < NUM_LEDS2; i++) {
        strip2.setPixelColor(i, strip2.Color(red, green, blue));
    }
    strip1.show();
    strip2.show();
}
