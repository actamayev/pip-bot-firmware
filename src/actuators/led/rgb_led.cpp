#include "rgb_led.h"

RgbLed rgbLed;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, ESP_LED_PIN, NEO_GRB + NEO_KHZ800);

LedAnimations ledAnimations(strip);

void RgbLed::turn_all_leds_off() {
    // Stop any running animations
    ledAnimations.stop_animation();
    // Set all LEDs to off
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();

    // Reset stored RGB values to match the off state
    _current_red = 0;
    _current_green = 0;
    _current_blue = 0;
}

void RgbLed::set_led_green() {
    set_main_board_leds_to_color(0, MAX_LED_BRIGHTNESS, 0);
}

void RgbLed::set_led_yellow() {
    set_main_board_leds_to_color(MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS, 0);
}

void RgbLed::turn_main_board_leds_off() {
    set_main_board_leds_to_color(0, 0, 0);
}

void RgbLed::set_default_colors(uint8_t red, uint8_t green, uint8_t blue) {
    // Set default color for all LEDs without showing them
    for (int i = 0; i < 8; i++) {
        if (i != 2 && i != 3) {
            default_colors[i][0] = red;
            default_colors[i][1] = green;
            default_colors[i][2] = blue;
            default_colors_set[i] = true;
        }
    }

    // Update current color tracking (for getCurrentRed/Green/Blue)
    _current_red = red;
    _current_green = green;
    _current_blue = blue;
}

void RgbLed::set_main_board_leds_to_color(uint8_t red, uint8_t green, uint8_t blue) {
    _current_red = red;
    _current_green = green;
    _current_blue = blue;

    // Stop any running animations
    ledAnimations.stop_animation();

    // Set default color for selected LEDs (0, 1, 4-7)
    int indices[] = {0, 1, 4, 5, 6, 7};
    for (int idx = 0; idx < 6; idx++) {
        int i = indices[idx];
        default_colors[i][0] = red;
        default_colors[i][1] = green;
        default_colors[i][2] = blue;
        default_colors_set[i] = true;
    }

    // Set only LEDs 0, 1, 4-7 to the same color (skip 2 and 3)
    for (int idx = 0; idx < 6; idx++) {
        int i = indices[idx];
        strip.setPixelColor(i, strip.Color(red, green, blue));
    }
    strip.show();
}

bool RgbLed::process_led_update(int led_index, uint8_t red, uint8_t green, uint8_t blue) {
    // Skip updates during rainbow animation
    if (ledAnimations.get_current_animation() == led_types::AnimationType::RAINBOW) {
        return false;
    }

    // Store the current animation type
    led_types::AnimationType current_anim = ledAnimations.get_current_animation();

    // Update the default color for this LED
    default_colors[led_index][0] = red;
    default_colors[led_index][1] = green;
    default_colors[led_index][2] = blue;
    default_colors_set[led_index] = true;

    // If we're in breathing or strobing mode, update the animation colors
    if (current_anim == led_types::AnimationType::BREATHING) {
        ledAnimations.update_breathing_color();
        return (led_index == 0); // Only return true for back_left LED (now index 0), which controls breathing color
    } else if (current_anim == led_types::AnimationType::STROBING) {
        ledAnimations.update_strobe_color();
        return (led_index == 0); // Only return true for back_left LED (now index 0), which controls strobe color
    } else {
        // For other cases (including NONE), stop animations
        ledAnimations.stop_animation();
        return true;
    }
}

void RgbLed::set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(0, red, green, blue)) return;

    strip.setPixelColor(0, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_top_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(1, red, green, blue)) return;

    strip.setPixelColor(1, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_right_headlight(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(2, red, green, blue)) return;

    strip.setPixelColor(2, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_left_headlight(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(3, red, green, blue)) return;

    strip.setPixelColor(3, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_top_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(4, red, green, blue)) return;

    strip.setPixelColor(4, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(5, red, green, blue)) return;

    strip.setPixelColor(5, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_back_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(6, red, green, blue)) return;

    strip.setPixelColor(6, strip.Color(red, green, blue));
    strip.show();
}

void RgbLed::set_back_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!process_led_update(7, red, green, blue)) return;

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
    constexpr uint8_t faint_blue = 20;                     // Very faint blue
    strip.setPixelColor(3, strip.Color(0, 0, faint_blue)); // left_headlight
    strip.setPixelColor(2, strip.Color(0, 0, faint_blue)); // right_headlight
    strip.show();
}

void RgbLed::turn_front_middle_leds_faint_blue() {
    constexpr uint8_t faint_blue = 20;                     // Very faint blue
    strip.setPixelColor(0, strip.Color(0, 0, faint_blue)); // middle_right
    strip.setPixelColor(1, strip.Color(0, 0, faint_blue)); // top_right
    strip.setPixelColor(4, strip.Color(0, 0, faint_blue)); // top_left
    strip.setPixelColor(5, strip.Color(0, 0, faint_blue)); // middle_left
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
    constexpr uint8_t faint_blue = 20;                     // Very faint blue
    strip.setPixelColor(6, strip.Color(0, 0, faint_blue)); // back_left
    strip.setPixelColor(7, strip.Color(0, 0, faint_blue)); // back_right
    strip.show();
}

void RgbLed::turn_back_leds_off() {
    strip.setPixelColor(6, strip.Color(0, 0, 0)); // back_left
    strip.setPixelColor(7, strip.Color(0, 0, 0)); // back_right
    strip.show();
}
