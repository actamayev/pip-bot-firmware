#include "./rgb_led.h"

RgbLed rgbLed;

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS1, ESP_LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS2, ESP_LED_PIN2, NEO_GRB + NEO_KHZ800);

LedAnimations ledAnimations(strip1, strip2);

void RgbLed::turn_led_off() {
    set_all_leds_to_color(0, 0, 0);
}

void RgbLed::set_led_red() {
    set_all_leds_to_color(MAX_LED_BRIGHTNESS, 0, 0);
}

void RgbLed::set_led_green() {
    set_all_leds_to_color(0, MAX_LED_BRIGHTNESS, 0);
}

void RgbLed::set_led_blue() {
    set_all_leds_to_color(0, 0, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_white() {
    set_all_leds_to_color(MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_purple() {
    set_all_leds_to_color(MAX_LED_BRIGHTNESS, 0, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_all_leds_to_color(uint8_t red, uint8_t green, uint8_t blue) {
    currentRed = red;
    currentGreen = green;
    currentBlue = blue;

    // Stop any running animations
    ledAnimations.stopAnimation();

    // Set default color for all LEDs
    for (int i = 0; i < 6; i++) {
        defaultColors[i][0] = red;
        defaultColors[i][1] = green;
        defaultColors[i][2] = blue;
        defaultColorsSet[i] = true;
    }

    // Set all LEDs to the same color
    for(int i = 0; i < strip1.numPixels(); i++) {
        strip1.setPixelColor(i, strip1.Color(red, green, blue));
    }
    for(int i = 0; i < strip2.numPixels(); i++) {
        strip2.setPixelColor(i, strip2.Color(red, green, blue));
    }
    strip1.show();
    strip2.show();
}

bool RgbLed::processLedUpdate(int ledIndex, uint8_t red, uint8_t green, uint8_t blue) {
    // Skip updates during rainbow animation
    if (ledAnimations.getCurrentAnimation() == LedAnimations::RAINBOW) {
        return false;
    }
    
    // Store the current animation type
    LedAnimations::AnimationType currentAnim = ledAnimations.getCurrentAnimation();
    
    // Update the default color for this LED
    defaultColors[ledIndex][0] = red;
    defaultColors[ledIndex][1] = green;
    defaultColors[ledIndex][2] = blue;
    defaultColorsSet[ledIndex] = true;
    
    // If we're in breathing or strobing mode, update the animation colors
    if (currentAnim == LedAnimations::BREATHING) {
        ledAnimations.updateBreathingColor();
        return (ledIndex == 0); // Only return true for top-left LED, which controls breathing color
    } 
    else if (currentAnim == LedAnimations::STROBING) {
        ledAnimations.updateStrobeColor();
        return (ledIndex == 0); // Only return true for top-left LED, which controls strobe color
    }
    else {
        // For other cases (including NONE), stop animations
        ledAnimations.stopAnimation();
        return true;
    }
}

void RgbLed::set_top_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(0, red, green, blue)) return;
    
    strip1.setPixelColor(0, strip1.Color(red, green, blue));
    strip1.show();
}

void RgbLed::set_top_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(1, red, green, blue)) return;
    
    strip1.setPixelColor(1, strip1.Color(red, green, blue));
    strip1.show();
}

void RgbLed::set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(2, red, green, blue)) return;
    
    strip2.setPixelColor(0, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(3, red, green, blue)) return;
    
    strip2.setPixelColor(1, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_back_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(4, red, green, blue)) return;
    
    strip2.setPixelColor(2, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_back_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(5, red, green, blue)) return;
    
    strip2.setPixelColor(3, strip2.Color(red, green, blue));
    strip2.show();
}
