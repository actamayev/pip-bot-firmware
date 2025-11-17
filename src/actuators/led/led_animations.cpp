#include "led_animations.h"

LedAnimations::LedAnimations(Adafruit_NeoPixel& strip) : _strip(strip) {}

void LedAnimations::start_breathing(int speed, float starting_brightness) {
    // Stop any current animation
    _current_animation = led_types::AnimationType::NONE;

    update_breathing_color();

    _breath_speed = speed;

    // Convert starting_brightness (0.0-1.0) to breath_progress
    // 0.0 (min) -> 1.5, 0.5 (mid) -> 0.0, 1.0 (max) -> 0.5
    if (starting_brightness <= 0.0f) {
        _breath_progress = 1.5f; // Start at minimum
    } else if (starting_brightness >= 1.0f) {
        _breath_progress = 0.5f; // Start at maximum
    } else {
        // Linear interpolation for values between 0 and 1
        _breath_progress = 1.5f - (starting_brightness * 1.5f);
    }

    // Set as current animation
    _current_animation = led_types::AnimationType::BREATHING;
    _is_paused = false;
    _is_fading_out = false;
    _last_breath_update = millis();
}

void LedAnimations::update_breathing_color() {
    // Use middle_right LED's default color for breathing (now index 0)
    if (!rgbLed.default_colors_set[0]) {
        _breath_min[0] = 0.1 * MAX_LED_BRIGHTNESS;
        _breath_min[1] = 0.1 * MAX_LED_BRIGHTNESS;
        _breath_min[2] = 0.1 * MAX_LED_BRIGHTNESS;

        _breath_max[0] = MAX_LED_BRIGHTNESS;
        _breath_max[1] = MAX_LED_BRIGHTNESS;
        _breath_max[2] = MAX_LED_BRIGHTNESS;
        return;
    }
    _breath_min[0] = 0.1 * rgbLed.default_colors[0][0]; // starts at 10% brightness
    _breath_min[1] = 0.1 * rgbLed.default_colors[0][1];
    _breath_min[2] = 0.1 * rgbLed.default_colors[0][2];

    _breath_max[0] = rgbLed.default_colors[0][0];
    _breath_max[1] = rgbLed.default_colors[0][1];
    _breath_max[2] = rgbLed.default_colors[0][2];
}

void LedAnimations::start_strobing(int speed) {
    // Stop any current animation
    _current_animation = led_types::AnimationType::NONE;

    update_strobe_color();

    _strobe_speed = speed;
    _strobe_state = false;

    // Set as current animation
    _current_animation = led_types::AnimationType::STROBING;
    _is_paused = false;
    _is_fading_out = false;
    _last_strobe_update = millis();
}

void LedAnimations::update_strobe_color() {
    // Use middle_right LED's default color for strobing (now index 0)
    if (!rgbLed.default_colors_set[0]) {
        _strobe_color[0] = MAX_LED_BRIGHTNESS;
        _strobe_color[1] = MAX_LED_BRIGHTNESS;
        _strobe_color[2] = MAX_LED_BRIGHTNESS;
        return;
    }
    _strobe_color[0] = rgbLed.default_colors[0][0];
    _strobe_color[1] = rgbLed.default_colors[0][1];
    _strobe_color[2] = rgbLed.default_colors[0][2];
}

void LedAnimations::start_rainbow(int cycle_time) {
    // Stop any current animation
    _current_animation = led_types::AnimationType::NONE;

    // Set rainbow parameters
    _rainbow_cycle_time = cycle_time;

    // Calculate step time based on cycle time
    _rainbow_step_time = _rainbow_cycle_time / 256;
    _rainbow_step_time = max(1UL, _rainbow_step_time); // Ensure minimum 1ms step time

    _rainbow_hue = 0;

    // Set as current animation
    _current_animation = led_types::AnimationType::RAINBOW;
    _is_paused = false;
    _is_fading_out = false;
    _last_rainbow_update = millis();
}

void LedAnimations::stop_animation() {
    _current_animation = led_types::AnimationType::NONE;
    _is_paused = false;
    _is_fading_out = false;
}

void LedAnimations::fade_out() {
    // Check if LEDs are already off (all stored RGB values are 0)
    if (rgbLed.get_current_red() == 0 && rgbLed.get_current_green() == 0 && rgbLed.get_current_blue() == 0) {
        // LEDs are already off, no need to fade
        return;
    }

    // Set fade parameters
    _breath_min[0] = 0;
    _breath_min[1] = 0;
    _breath_min[2] = 0;

    // Set max to current stored values
    _breath_max[0] = rgbLed.get_current_red();
    _breath_max[1] = rgbLed.get_current_green();
    _breath_max[2] = rgbLed.get_current_blue();

    // Start at 0.5 to begin fade from maximum brightness and go straight to min
    _breath_progress = 0.5;
    _breath_speed = 500; // Faster fade out

    // Set to breathing for fade out
    _current_animation = led_types::AnimationType::BREATHING;
    _is_fading_out = true;
    _is_paused = false;
    _last_breath_update = millis();
}

void LedAnimations::update() {
    if (_is_paused || _current_animation == led_types::AnimationType::NONE) {
        return;
    }

    switch (_current_animation) {
        case led_types::AnimationType::BREATHING:
            update_breathing();
            break;
        case led_types::AnimationType::STROBING:
            update_strobing();
            break;
        case led_types::AnimationType::RAINBOW:
            update_rainbow();
            break;
        default:
            break;
    }
}

void LedAnimations::update_breathing() {
    uint32_t current_time = millis();
    uint32_t time_per_step = _breath_speed / 255;
    time_per_step = max(1UL, time_per_step);

    if (current_time - _last_breath_update < time_per_step) return;
    _last_breath_update = current_time;

    float factor;

    // Calculate the breathing factor using sine wave
    factor = (sin(_breath_progress * PI) + 1.0) / 2.0;

    // Interpolate between min and max for each color
    uint8_t r = _breath_min[0] + factor * (_breath_max[0] - _breath_min[0]);
    uint8_t g = _breath_min[1] + factor * (_breath_max[1] - _breath_min[1]);
    uint8_t b = _breath_min[2] + factor * (_breath_max[2] - _breath_min[2]);

    // Set the color
    set_all_leds(r, g, b);

    // Update progress
    float progress_step = 1.0 / 255.0;
    _breath_progress += progress_step;

    // Check if we're fading out and have reached the bottom of the cycle
    if (_is_fading_out && _breath_progress >= 1.0) {
        // We've faded out completely, stop breathing and ensure LEDs are off
        _current_animation = led_types::AnimationType::NONE;
        _is_fading_out = false;
        set_all_leds(0, 0, 0);
        return;
    }

    // For normal breathing, reset when we complete a full cycle
    if (_breath_progress >= 2.0) {
        _breath_progress = 0.0;
    }
}

void LedAnimations::update_strobing() {
    uint32_t current_time = millis();

    if (current_time - _last_strobe_update >= _strobe_speed) {
        _last_strobe_update = current_time;
        _strobe_state = !_strobe_state;

        if (_strobe_state) {
            // ON state - use the set color
            set_all_leds(_strobe_color[0], _strobe_color[1], _strobe_color[2]);
        } else {
            // OFF state
            set_all_leds(0, 0, 0);
        }
    }
}

void LedAnimations::update_rainbow() {
    uint32_t current_time = millis();

    if (current_time - _last_rainbow_update < _rainbow_step_time) return;
    _last_rainbow_update = current_time;

    // Slowly rotate the entire rainbow pattern
    _rainbow_hue = (_rainbow_hue + 1) % 256;

    // Define base hues for a 6-LED rainbow (excluding headlights)
    uint8_t base_hues[6] = {
        0,   // Red
        51,  // Orange
        102, // Yellow
        153, // Green
        204, // Blue
        255  // Violet
    };

    // Map colors to each LED EXCLUDING headlights (indices 2,3):
    _strip.setPixelColor(0, color_hsv((base_hues[0] + _rainbow_hue) % 256)); // middle_right
    _strip.setPixelColor(1, color_hsv((base_hues[5] + _rainbow_hue) % 256)); // top_right
    // Skip indices 2 and 3 (headlights)
    _strip.setPixelColor(4, color_hsv((base_hues[4] + _rainbow_hue) % 256)); // top_left
    _strip.setPixelColor(5, color_hsv((base_hues[3] + _rainbow_hue) % 256)); // middle_left
    _strip.setPixelColor(6, color_hsv((base_hues[2] + _rainbow_hue) % 256)); // back_left
    _strip.setPixelColor(7, color_hsv((base_hues[1] + _rainbow_hue) % 256)); // back_right

    _strip.show();
}

uint32_t LedAnimations::color_hsv(uint8_t h, uint8_t s, uint8_t v) {
    // Convert HSV to RGB
    uint8_t region, remainder, p, q, t;
    uint8_t r, g, b;

    // Modified for 8 regions instead of 6
    region = h / 32;                     // Changed from 43 to 32 (256/8 = 32)
    remainder = (h - (region * 32)) * 8; // Changed from 6 to 8

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
        case 6:
            r = v;
            g = p;
            b = t;
            break;
        case 7:
            r = q;
            g = p;
            b = v;
            break;
    }

    return _strip.Color(r, g, b);
}

void LedAnimations::turn_off() {
    _current_animation = led_types::AnimationType::NONE;
    rgbLed.turn_main_board_leds_off();
}

void LedAnimations::set_all_leds(uint8_t red, uint8_t green, uint8_t blue) {
    // Set all LEDs except headlights (indices 2 and 3) to the same color
    for (int i = 0; i < _strip.numPixels(); i++) {
        if (i != 2 && i != 3) { // Skip headlight indices
            _strip.setPixelColor(i, _strip.Color(red, green, blue));
        }
    }
    _strip.show();
}
