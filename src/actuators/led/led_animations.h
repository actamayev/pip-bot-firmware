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
    void startBreathing(int speed = 2000, float startingBrightness = 0.5F);
    void startStrobing(int speed = 500);
    void startRainbow(int cycleTime = 2000);
    void turnOff();

    // Manage animations
    void stopAnimation();
    void fadeOut();
    void update();

    // Get current animation state
    led_types::AnimationType getCurrentAnimation() const {
        return _currentAnimation;
    }

    // Update animation colors
    void updateBreathingColor();
    void updateStrobeColor();

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
    void updateBreathing();
    void updateStrobing();
    void updateRainbow();
    uint32_t colorHSV(uint8_t h, uint8_t s = MAX_LED_BRIGHTNESS, uint8_t v = MAX_LED_BRIGHTNESS);
    void setAllLeds(uint8_t red, uint8_t green, uint8_t blue);
};

extern LedAnimations ledAnimations;
