#pragma once

#include <Adafruit_NeoPixel.h>

#include <cmath>

#include "rgb_led.h"
#include "utils/config.h"
#include "utils/structs.h"

class LedAnimations {
  public:
    explicit LedAnimations(Adafruit_NeoPixel& strip);

    // Set the current animation
    void start_breathing(int speed = 2000, float starting_brightness = 0.5F);
    void start_strobing(int speed = 500);
    void start_rainbow(int cycle_time = 2000);
    void turn_off();

    // Manage animations
    void stop_animation();
    void fade_out();
    void update();

    // Get current animation state
    led_types::AnimationType get_current_animation() const {
        return _current_animation;
    }

    // Update animation colors
    void update_breathing_color();
    void update_strobe_color();

  private:
    Adafruit_NeoPixel& _strip;

    // Current animation state
    led_types::AnimationType _current_animation = led_types::AnimationType::NONE;
    bool _is_paused = false;
    bool _is_fading_out = false;

    // Breathing animation parameters
    uint8_t _breath_min[3] = {0, 0, 0};
    uint8_t _breath_max[3] = {0, 0, 0};
    float _breath_progress = 0.0;
    int _breath_speed = 2000;
    uint32_t _last_breath_update = 0;

    // Strobing animation parameters
    uint8_t _strobe_color[3] = {0, 0, 0};
    int _strobe_speed = 500;
    bool _strobe_state = false;
    uint32_t _last_strobe_update = 0;

    // Rainbow animation parameters
    int _rainbow_cycle_time = 2000;
    uint32_t _rainbow_step_time = 0;
    uint8_t _rainbow_hue = 0;
    uint32_t _last_rainbow_update = 0;

    // Helper methods
    void update_breathing();
    void update_strobing();
    void update_rainbow();
    static uint32_t color_hsv(uint8_t h, uint8_t s = MAX_LED_BRIGHTNESS, uint8_t v = MAX_LED_BRIGHTNESS);
    void set_all_leds(uint8_t red, uint8_t green, uint8_t blue);
};

extern LedAnimations led_animations;
