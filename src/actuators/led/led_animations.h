#pragma once

#include <Adafruit_NeoPixel.h>
#include <math.h>

#include "rgb_led.h"
#include "utils/config.h"
#include "utils/structs.h"

class LedAnimations {
  public:
    explicit LedAnimations(Adafruit_NeoPixel& strip);

    // Set the current animation
    void start_breathing(int speed = 2000, float startingBrightness = 0.5F);
    void start_strobing(int speed = 500);
    void start_rainbow(int cycleTime = 2000);
    void turn_off();

    // Manage animations
    void stop_animation();
    void fade_out();
    void update();

    // Get current animation state
    led_types::AnimationType get_current_animation() const {
        return _currentAnimation;
    }

    // Update animation colors
    void update_breathing_color();
    void update_strobe_color();

  private:
    Adafruit_NeoPixel& _strip;

    // Current animation state
    led_types::AnimationType _currentAnimation = led_types::AnimationType::NONE;
    bool _isPaused = false;
    bool _isFadingOut = false;

    // Breathing animation parameters
    uint8_t _breathMin[3] = {0, 0, 0};
    uint8_t _breathMax[3] = {0, 0, 0};
    float _breathProgress = 0.0;
    int _breathSpeed = 2000;
    unsigned long _lastBreathUpdate = 0;

    // Strobing animation parameters
    uint8_t _strobeColor[3] = {0, 0, 0};
    int _strobeSpeed = 500;
    bool _strobeState = false;
    unsigned long _lastStrobeUpdate = 0;

    // Rainbow animation parameters
    int _rainbowCycleTime = 2000;
    unsigned long _rainbowStepTime = 0;
    uint8_t _rainbowHue = 0;
    unsigned long _lastRainbowUpdate = 0;

    // Helper methods
    void update_breathing();
    void update_strobing();
    void update_rainbow();
    uint32_t color_hsv(uint8_t h, uint8_t s = MAX_LED_BRIGHTNESS, uint8_t v = MAX_LED_BRIGHTNESS);
    void set_all_leds(uint8_t red, uint8_t green, uint8_t blue);
};

extern LedAnimations ledAnimations;
