#include "./include/config.h"
#include "./include/rgb_led.h"

RgbLed rgbLed;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, ESP_LED_PIN, NEO_GRB + NEO_KHZ800);

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

void RgbLed::set_led_to_color(int red, int green, int blue) {
    strip.setPixelColor(0, strip.Color(red, green, blue));
    strip.show();
}
