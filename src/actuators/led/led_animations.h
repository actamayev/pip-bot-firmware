#pragma once

#include <Adafruit_NeoPixel.h>
#include "../../utils/config.h"
#include "./rgb_led.h"

class LedAnimations {
    public:
        LedAnimations(Adafruit_NeoPixel& strip1, Adafruit_NeoPixel& strip2);
        
        // Animation types
        enum AnimationType {
            NONE,
            BREATHING,
            STROBING,
            RAINBOW
        };
        
        // Set the current animation
        void startBreathing(int speed = 2000);
        void startStrobing(int speed = 100);
        void startRainbow(int cycleTime = 2000);
        void stopBreathing();
        
        // Manage animations
        void stopAnimation();
        void pauseAnimation();
        void fadeOut();
        void update();
        
        // Get current animation state
        AnimationType getCurrentAnimation() const { return currentAnimation; }
        bool isAnimating() const { return currentAnimation != NONE; }
        
    private:
        Adafruit_NeoPixel& strip1;
        Adafruit_NeoPixel& strip2;
        
        // Current animation state
        AnimationType currentAnimation = NONE;
        bool isPaused = false;
        bool isFadingOut = false;
        
        // Breathing animation parameters
        uint8_t breathMin[3] = {0, 0, 0};
        uint8_t breathMax[3] = {0, 0, 0};
        float breathProgress = 0.0;
        int breathSpeed = 2000;
        unsigned long lastBreathUpdate = 0;
        
        // Strobing animation parameters
        uint8_t strobeColor[3] = {0, 0, 0};
        int strobeSpeed = 100;
        bool strobeState = false;
        unsigned long lastStrobeUpdate = 0;
        
        // Rainbow animation parameters
        int rainbowCycleTime = 2000;
        unsigned long rainbowStepTime = 0;
        uint8_t rainbowHue = 0;
        unsigned long lastRainbowUpdate = 0;
        
        // Helper methods
        void updateBreathing();
        void updateStrobing();
        void updateRainbow();
        uint32_t colorHSV(uint8_t h, uint8_t s = maxBrightness, uint8_t v = maxBrightness);
        void setAllLeds(uint8_t red, uint8_t green, uint8_t blue);

        void updateBreathingColor();
        void updateStrobeColor();

        const static int maxBrightness = 150;
};

extern LedAnimations ledAnimations;
