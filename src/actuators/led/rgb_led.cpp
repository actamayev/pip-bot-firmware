#include "rgb_led.h"

RgbLed rgbLed;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, ESP_LED_PIN, NEO_GRB + NEO_KHZ800);

LedAnimations ledAnimations(strip);

void RgbLed::turn_all_leds_off() {
    // Stop any running animations
    ledAnimations.stopAnimation();
    // Set all LEDs to off
    for(int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
}

void RgbLed::set_led_red() {
    set_main_board_leds_to_color(MAX_LED_BRIGHTNESS, 0, 0);
}

void RgbLed::set_led_green() {
    set_main_board_leds_to_color(0, MAX_LED_BRIGHTNESS, 0);
}

void RgbLed::set_led_blue() {
    set_main_board_leds_to_color(0, 0, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_white() {
    set_main_board_leds_to_color(MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_purple() {
    set_main_board_leds_to_color(MAX_LED_BRIGHTNESS, 0, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_yellow() {
    set_main_board_leds_to_color(MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS, 0);
}

void RgbLed::turn_main_board_leds_off() {
    set_main_board_leds_to_color(0, 0, 0);
}

void RgbLed::setDefaultColors(uint8_t red, uint8_t green, uint8_t blue) {
    // Set default color for all LEDs without showing them
    for (int i = 0; i < 8; i++) {
        if (i != 2 && i != 3) {
            defaultColors[i][0] = red;
            defaultColors[i][1] = green;
            defaultColors[i][2] = blue;
            defaultColorsSet[i] = true;
        }
    }

    // Update current color tracking (for getCurrentRed/Green/Blue)
    currentRed = red;
    currentGreen = green;
    currentBlue = blue;
}

void RgbLed::set_main_board_leds_to_color(uint8_t red, uint8_t green, uint8_t blue) {
    currentRed = red;
    currentGreen = green;
    currentBlue = blue;

    // Stop any running animations
    ledAnimations.stopAnimation();

    // Set default color for selected LEDs (0, 1, 4-7)
    int indices[] = {0, 1, 4, 5, 6, 7};
    for (int idx = 0; idx < 6; idx++) {
        int i = indices[idx];
        defaultColors[i][0] = red;
        defaultColors[i][1] = green;
        defaultColors[i][2] = blue;
        defaultColorsSet[i] = true;
    }

    // Set only LEDs 0, 1, 4-7 to the same color (skip 2 and 3)
    for (int idx = 0; idx < 6; idx++) {
        int i = indices[idx];
        strip.setPixelColor(i, strip.Color(red, green, blue));
    }
    strip.show();
}

bool RgbLed::processLedUpdate(int ledIndex, uint8_t red, uint8_t green, uint8_t blue) {
    // Skip updates during rainbow animation
    if (ledAnimations.getCurrentAnimation() == LedTypes::RAINBOW) {
        return false;
    }
    
    // Store the current animation type
    LedTypes::AnimationType currentAnim = ledAnimations.getCurrentAnimation();
    
    // Update the default color for this LED
    defaultColors[ledIndex][0] = red;
    defaultColors[ledIndex][1] = green;
    defaultColors[ledIndex][2] = blue;
    defaultColorsSet[ledIndex] = true;
    
    // If we're in breathing or strobing mode, update the animation colors
    if (currentAnim == LedTypes::BREATHING) {
        ledAnimations.updateBreathingColor();
        return (ledIndex == 0); // Only return true for back_left LED (now index 0), which controls breathing color
    } 
    else if (currentAnim == LedTypes::STROBING) {
        ledAnimations.updateStrobeColor();
        return (ledIndex == 0); // Only return true for back_left LED (now index 0), which controls strobe color
    } else {
        // For other cases (including NONE), stop animations
        ledAnimations.stopAnimation();
        return true;
    }
}

void RgbLed::set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(0, red, green, blue)) return;
    
    strip.setPixelColor(0, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_top_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(1, red, green, blue)) return;
    
    strip.setPixelColor(1, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_right_headlight(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(2, red, green, blue)) return;
    
    strip.setPixelColor(2, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_left_headlight(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(3, red, green, blue)) return;
    
    strip.setPixelColor(3, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_top_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(4, red, green, blue)) return;
    
    strip.setPixelColor(4, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(5, red, green, blue)) return;
    
    strip.setPixelColor(5, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_back_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(6, red, green, blue)) return;
    
    strip.setPixelColor(6, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_back_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(7, red, green, blue)) return;
    
    strip.setPixelColor(7, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::turn_headlights_on() {
    // Set headlights to white (swapped indices due to reversal)
    strip.setPixelColor(3, strip.Color(255, 255, 255)); // left_headlight
    strip.setPixelColor(2, strip.Color(255, 255, 255)); // right_headlight
    strip.show();
}

void RgbLed::turn_headlights_off() {
    strip.setPixelColor(3, strip.Color(0, 0, 0)); // left_headlight
    strip.setPixelColor(2, strip.Color(0, 0, 0)); // right_headlight
    strip.show();
}

void RgbLed::turn_headlights_faint_blue() {
    constexpr uint8_t faintBlue = 20; // Very faint blue
    strip.setPixelColor(3, strip.Color(0, 0, faintBlue)); // left_headlight
    strip.setPixelColor(2, strip.Color(0, 0, faintBlue)); // right_headlight
    strip.show();
}

void RgbLed::turn_front_middle_leds_faint_blue() {
    constexpr uint8_t faintBlue = 20; // Very faint blue
    strip.setPixelColor(0, strip.Color(0, 0, faintBlue)); // middle_right
    strip.setPixelColor(1, strip.Color(0, 0, faintBlue)); // top_right
    strip.setPixelColor(4, strip.Color(0, 0, faintBlue)); // top_left
    strip.setPixelColor(5, strip.Color(0, 0, faintBlue)); // middle_left
    strip.show();
}

void RgbLed::turn_front_middle_leds_off() {
    strip.setPixelColor(0, strip.Color(0, 0, 0)); // middle_right
    strip.setPixelColor(1, strip.Color(0, 0, 0)); // top_right
    strip.setPixelColor(4, strip.Color(0, 0, 0)); // top_left
    strip.setPixelColor(5, strip.Color(0, 0, 0)); // middle_left
    strip.show();
}

void RgbLed::turn_back_leds_faint_blue() {
    constexpr uint8_t faintBlue = 20; // Very faint blue
    strip.setPixelColor(6, strip.Color(0, 0, faintBlue)); // back_left
    strip.setPixelColor(7, strip.Color(0, 0, faintBlue)); // back_right
    strip.show();
}

void RgbLed::turn_back_leds_off() {
    strip.setPixelColor(6, strip.Color(0, 0, 0)); // back_left
    strip.setPixelColor(7, strip.Color(0, 0, 0)); // back_right
    strip.show();
}
