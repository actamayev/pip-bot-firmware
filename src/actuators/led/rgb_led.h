#pragma once

#include <Adafruit_NeoPixel.h>

#include "led_animations.h"
#include "utils/config.h"
#include "utils/structs.h"

constexpr uint8_t ESP_LED_PIN = 4;
constexpr uint8_t NUM_LEDS = 8;

class RgbLed {
  public:
    // Basic color controls
    void turn_all_leds_off();
    void set_led_green();
    void set_led_yellow();
    void set_default_colors(uint8_t red, uint8_t green, uint8_t blue);

    // Individual LED controls
    void set_top_left_led(uint8_t red, uint8_t green, uint8_t blue);
    void set_top_right_led(uint8_t red, uint8_t green, uint8_t blue);
    void set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue);
    void set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue);
    void set_back_left_led(uint8_t red, uint8_t green, uint8_t blue);
    void set_back_right_led(uint8_t red, uint8_t green, uint8_t blue);
    void set_main_board_leds_to_color(uint8_t red, uint8_t green, uint8_t blue);
    void set_right_headlight(uint8_t red, uint8_t green, uint8_t blue);
    void set_left_headlight(uint8_t red, uint8_t green, uint8_t blue);

    // Get current color values
    uint8_t get_current_red() const {
        return _currentRed;
    }
    uint8_t get_current_green() const {
        return _currentGreen;
    }
    uint8_t get_current_blue() const {
        return _currentBlue;
    }

    // Default colors for each LED (REVERSED ORDER)
    uint8_t defaultColors[8][3] = {
        {0, 0, 0}, // middle_right      - R,G,B
        {0, 0, 0}, // top_right    - R,G,B
        {0, 0, 0}, // right_headlight       - R,G,B
        {0, 0, 0}, // left_headlight - R,G,B
        {0, 0, 0}, // top_left - R,G,B
        {0, 0, 0}, // middle_left      - R,G,B
        {0, 0, 0}, // back_left   - R,G,B
        {0, 0, 0}  // back_right     - R,G,B
    };
    bool defaultColorsSet[8] = {false, false, false, false, false, false, false, false};

    void turn_headlights_on();
    void turn_headlights_off();
    void turn_headlights_faint_blue();

    void turn_front_middle_leds_faint_blue();
    void turn_front_middle_leds_off();

    void turn_back_leds_faint_blue();
    void turn_back_leds_off();

    void turn_main_board_leds_off();

  private:
    bool process_led_update(int ledIndex, uint8_t red, uint8_t green, uint8_t blue);

    // Current LED state
    uint8_t _currentRed = 0;
    uint8_t _currentGreen = 0;
    uint8_t _currentBlue = 0;

    // TODO: Bring this back later, along with the captureCurrentState and restoreCapturedState functions (see 7/22/25 PR)
    LedState _capturedState{};
};

extern RgbLed rgbLed;
