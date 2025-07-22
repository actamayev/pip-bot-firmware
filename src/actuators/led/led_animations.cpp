#include "led_animations.h"

LedAnimations::LedAnimations(Adafruit_NeoPixel& strip)
    : strip(strip) {
}

void LedAnimations::startBreathing(int speed, float startingBrightness) {
    // Stop any current animation
    currentAnimation = LedTypes::NONE;
    
    updateBreathingColor();
    
    breathSpeed = speed;
    
    // Convert startingBrightness (0.0-1.0) to breathProgress
    // 0.0 (min) -> 1.5, 0.5 (mid) -> 0.0, 1.0 (max) -> 0.5
    if (startingBrightness <= 0.0f) {
        breathProgress = 1.5f;  // Start at minimum
    } else if (startingBrightness >= 1.0f) {
        breathProgress = 0.5f;  // Start at maximum  
    } else {
        // Linear interpolation for values between 0 and 1
        breathProgress = 1.5f - (startingBrightness * 1.5f);
    }
    
    // Set as current animation
    currentAnimation = LedTypes::BREATHING;
    isPaused = false;
    isFadingOut = false;
    lastBreathUpdate = millis();
}

void LedAnimations::updateBreathingColor() {
    // Use back_left LED's default color for breathing (now index 0)
    if (!rgbLed.defaultColorsSet[0]) {
        breathMin[0] = 0.1 * MAX_LED_BRIGHTNESS;
        breathMin[1] = 0.1 * MAX_LED_BRIGHTNESS;
        breathMin[2] = 0.1 * MAX_LED_BRIGHTNESS;

        breathMax[0] = MAX_LED_BRIGHTNESS;
        breathMax[1] = MAX_LED_BRIGHTNESS;
        breathMax[2] = MAX_LED_BRIGHTNESS;
        return;
    }
    breathMin[0] = 0.1 * rgbLed.defaultColors[0][0]; // starts at 10% brightness
    breathMin[1] = 0.1 * rgbLed.defaultColors[0][1];
    breathMin[2] = 0.1 * rgbLed.defaultColors[0][2];

    breathMax[0] = rgbLed.defaultColors[0][0];
    breathMax[1] = rgbLed.defaultColors[0][1];
    breathMax[2] = rgbLed.defaultColors[0][2];
}

void LedAnimations::startStrobing(int speed) {
    // Stop any current animation
    currentAnimation = LedTypes::NONE;
    
    updateStrobeColor();
    
    strobeSpeed = speed;
    strobeState = false;
    
    // Set as current animation
    currentAnimation = LedTypes::STROBING;
    isPaused = false;
    isFadingOut = false;
    lastStrobeUpdate = millis();
}

void LedAnimations::updateStrobeColor() {
    // Use back_left LED's default color for strobing (now index 0)
    if (!rgbLed.defaultColorsSet[0]) {
        strobeColor[0] = MAX_LED_BRIGHTNESS;
        strobeColor[1] = MAX_LED_BRIGHTNESS;
        strobeColor[2] = MAX_LED_BRIGHTNESS;
        return;
    }
    strobeColor[0] = rgbLed.defaultColors[0][0];
    strobeColor[1] = rgbLed.defaultColors[0][1];
    strobeColor[2] = rgbLed.defaultColors[0][2];
}

void LedAnimations::startRainbow(int cycleTime) {
    // Stop any current animation
    currentAnimation = LedTypes::NONE;
    
    // Set rainbow parameters
    rainbowCycleTime = cycleTime;
    
    // Calculate step time based on cycle time
    rainbowStepTime = rainbowCycleTime / 256;
    rainbowStepTime = max(1UL, rainbowStepTime); // Ensure minimum 1ms step time
    
    rainbowHue = 0;
    
    // Set as current animation
    currentAnimation = LedTypes::RAINBOW;
    isPaused = false;
    isFadingOut = false;
    lastRainbowUpdate = millis();
}

void LedAnimations::stopAnimation() {
    currentAnimation = LedTypes::NONE;
    isPaused = false;
    isFadingOut = false;
}

void LedAnimations::fadeOut() {
    // Set fade parameters
    breathMin[0] = 0;
    breathMin[1] = 0;
    breathMin[2] = 0;
    
    breathMax[0] = rgbLed.getCurrentRed();
    breathMax[1] = rgbLed.getCurrentGreen();
    breathMax[2] = rgbLed.getCurrentBlue();
    
    breathProgress = 0.0;
    breathSpeed = 500; // Faster fade out
    
    // Set to breathing for fade out
    currentAnimation = LedTypes::BREATHING;
    isFadingOut = true;
    isPaused = false;
    lastBreathUpdate = millis();
}

void LedAnimations::update() {
    if (isPaused || currentAnimation == LedTypes::NONE) {
        return;
    }
    
    switch (currentAnimation) {
        case LedTypes::BREATHING:
            updateBreathing();
            break;
        case LedTypes::STROBING:
            updateStrobing();
            break;
        case LedTypes::RAINBOW:
            updateRainbow();
            break;
        default:
            break;
    }
}

void LedAnimations::updateBreathing() {
    unsigned long currentTime = millis();
    unsigned long timePerStep = breathSpeed / 255;
    timePerStep = max(1UL, timePerStep);
    
    if (currentTime - lastBreathUpdate < timePerStep) return;
    lastBreathUpdate = currentTime;
    
    float factor;
    
    // Calculate the breathing factor using sine wave
    factor = (sin(breathProgress * PI) + 1.0) / 2.0;
    
    // Interpolate between min and max for each color
    uint8_t r = breathMin[0] + factor * (breathMax[0] - breathMin[0]);
    uint8_t g = breathMin[1] + factor * (breathMax[1] - breathMin[1]);
    uint8_t b = breathMin[2] + factor * (breathMax[2] - breathMin[2]);
    
    // Set the color
    setAllLeds(r, g, b);
    
    // Update progress
    float progressStep = 1.0 / 255.0;
    breathProgress += progressStep;
    
    // Check if we're fading out and have reached the bottom of the cycle
    if (isFadingOut && breathProgress >= 1.0) {
        // We've faded out completely, stop breathing and ensure LEDs are off
        currentAnimation = LedTypes::NONE;
        isFadingOut = false;
        setAllLeds(0, 0, 0);
        return;
    }
    
    // For normal breathing, reset when we complete a full cycle
    if (breathProgress >= 2.0) {
        breathProgress = 0.0;
    }
}

void LedAnimations::updateStrobing() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastStrobeUpdate >= strobeSpeed) {
        lastStrobeUpdate = currentTime;
        strobeState = !strobeState;
        
        if (strobeState) {
            // ON state - use the set color
            setAllLeds(strobeColor[0], strobeColor[1], strobeColor[2]);
        } else {
            // OFF state
            setAllLeds(0, 0, 0);
        }
    }
}

void LedAnimations::updateRainbow() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastRainbowUpdate < rainbowStepTime) return;
    lastRainbowUpdate = currentTime;
    
    // Slowly rotate the entire rainbow pattern
    rainbowHue = (rainbowHue + 1) % 256;
    
    // Define base hues for a 6-LED rainbow (excluding headlights)
    uint8_t baseHues[6] = {
        0,    // Red
        51,   // Orange  
        102,  // Yellow
        153,  // Green
        204,  // Blue
        255   // Violet
    };
    
    // Map colors to each LED EXCLUDING headlights (indices 2,3):
    strip.setPixelColor(0, colorHSV((baseHues[0] + rainbowHue) % 256)); // middle_right
    strip.setPixelColor(1, colorHSV((baseHues[5] + rainbowHue) % 256)); // top_right
    // Skip indices 2 and 3 (headlights)
    strip.setPixelColor(4, colorHSV((baseHues[4] + rainbowHue) % 256)); // top_left
    strip.setPixelColor(5, colorHSV((baseHues[3] + rainbowHue) % 256)); // middle_left
    strip.setPixelColor(6, colorHSV((baseHues[2] + rainbowHue) % 256)); // back_left
    strip.setPixelColor(7, colorHSV((baseHues[1] + rainbowHue) % 256)); // back_right
    
    strip.show();
}

uint32_t LedAnimations::colorHSV(uint8_t h, uint8_t s, uint8_t v) {
    // Convert HSV to RGB
    uint8_t region, remainder, p, q, t;
    uint8_t r, g, b;
    
    // Modified for 8 regions instead of 6
    region = h / 32;  // Changed from 43 to 32 (256/8 = 32)
    remainder = (h - (region * 32)) * 8;  // Changed from 6 to 8
    
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
    
    switch (region) {
        case 0:
            r = v; g = t; b = p;
            break;
        case 1:
            r = q; g = v; b = p;
            break;
        case 2:
            r = p; g = v; b = t;
            break;
        case 3:
            r = p; g = q; b = v;
            break;
        case 4:
            r = t; g = p; b = v;
            break;
        case 5:
            r = v; g = p; b = q;
            break;
        case 6:
            r = v; g = p; b = t;
            break;
        case 7:
            r = q; g = p; b = v;
            break;
    }
    
    return strip.Color(r, g, b);
}

void LedAnimations::turnOff() {
    currentAnimation = LedTypes::NONE;
    rgbLed.turn_all_leds_off();
}

void LedAnimations::setAllLeds(uint8_t red, uint8_t green, uint8_t blue) {
    // Set all LEDs except headlights (indices 2 and 3) to the same color
    for(int i = 0; i < strip.numPixels(); i++) {
        if (i != 2 && i != 3) {  // Skip headlight indices
            strip.setPixelColor(i, strip.Color(red, green, blue));
        }
    }
    strip.show();
}
