#include "led_animations.h"
#include <math.h>

LedAnimations::LedAnimations(Adafruit_NeoPixel& strip1, Adafruit_NeoPixel& strip2)
    : strip1(strip1), strip2(strip2) {
}

void LedAnimations::startBreathing(int speed) {
    // Stop any current animation
    currentAnimation = NONE;
    
    updateBreathingColor();
    
    breathSpeed = speed;
    breathProgress = 0.0;
    
    // Set as current animation
    currentAnimation = BREATHING;
    isPaused = false;
    isFadingOut = false;
    lastBreathUpdate = millis();
}

void LedAnimations::updateBreathingColor() {
    // Use top-left LED's default color for breathing
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
    currentAnimation = NONE;
    
    updateStrobeColor();
    
    strobeSpeed = speed;
    strobeState = false;
    
    // Set as current animation
    currentAnimation = STROBING;
    isPaused = false;
    isFadingOut = false;
    lastStrobeUpdate = millis();
}

void LedAnimations::updateStrobeColor() {
    // Use top-left LED's default color for strobing
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
    currentAnimation = NONE;
    
    // Set rainbow parameters
    rainbowCycleTime = cycleTime;
    
    // Calculate step time based on cycle time
    rainbowStepTime = rainbowCycleTime / 256;
    rainbowStepTime = max(1UL, rainbowStepTime); // Ensure minimum 1ms step time
    
    rainbowHue = 0;
    
    // Set as current animation
    currentAnimation = RAINBOW;
    isPaused = false;
    isFadingOut = false;
    lastRainbowUpdate = millis();
}

void LedAnimations::startSnake(int speed) {
    // Stop any current animation
    currentAnimation = NONE;
    
    updateSnakeColor();
    
    snakeSpeed = speed;
    snakeHeadPosition = 0;
    
    // Set as current animation
    currentAnimation = SNAKE;
    isPaused = false;
    isFadingOut = false;
    lastSnakeUpdate = millis();
}

void LedAnimations::updateSnakeColor() {
    // Use current RGB LED color for the snake head
    snakeColor[0] = rgbLed.getCurrentRed();
    snakeColor[1] = rgbLed.getCurrentGreen();
    snakeColor[2] = rgbLed.getCurrentBlue();
    
    // If current color is 0,0,0, use white
    if (snakeColor[0] == 0 && snakeColor[1] == 0 && snakeColor[2] == 0) {
        snakeColor[0] = MAX_LED_BRIGHTNESS;
        snakeColor[1] = MAX_LED_BRIGHTNESS;
        snakeColor[2] = MAX_LED_BRIGHTNESS;
    }
}

void LedAnimations::updateSnake() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastSnakeUpdate < snakeSpeed) return;
    lastSnakeUpdate = currentTime;
    
    // Move the snake head position
    snakeHeadPosition = (snakeHeadPosition + 1) % 6;
    
    // First clear all LEDs
    setAllLeds(0, 0, 0);
    
    // Define the LED mapping (which strip and pixel for each snake position)
    struct LedMapping {
        int stripIndex; // 1 for strip1, 2 for strip2
        int pixelIndex; // pixel index within the strip
    };
    
    LedMapping ledMap[6] = {
        {1, 1}, // Position 0: Top right    - strip1, pixel 1
        {1, 0}, // Position 1: Top left     - strip1, pixel 0
        {2, 0}, // Position 2: Middle left  - strip2, pixel 0
        {2, 2}, // Position 3: Bottom left  - strip2, pixel 2
        {2, 3}, // Position 4: Bottom right - strip2, pixel 3
        {2, 1}  // Position 5: Middle right - strip2, pixel 1
    };
    
    // For each position in the snake (0=head, 5=tail)
    for (int snakePos = 0; snakePos < 6; snakePos++) {
        // Calculate which physical position this part of the snake is at
        int physicalPos = (snakeHeadPosition + snakePos) % 6;
        
        // Calculate brightness factor (100%, 80%, 60%, 40%, 20%, 10%)
        float factor;
        if (snakePos == 0) {
            factor = 1.0; // Head at 100%
        } else if (snakePos == 5) {
            factor = 0.1; // Tail at 10%
        } else {
            factor = 1.0 - (snakePos * 0.2); // Linear decrease
        }
        
        // Calculate brightness-adjusted color
        uint8_t r = factor * snakeColor[0];
        uint8_t g = factor * snakeColor[1];
        uint8_t b = factor * snakeColor[2];
        
        // Get the strip and pixel for this physical position
        LedMapping mapping = ledMap[physicalPos];
        
        // Set the LED
        if (mapping.stripIndex == 1) {
            strip1.setPixelColor(mapping.pixelIndex, strip1.Color(r, g, b));
        } else {
            strip2.setPixelColor(mapping.pixelIndex, strip2.Color(r, g, b));
        }
    }
    
    // Show the updated strips
    strip1.show();
    strip2.show();
}

void LedAnimations::stopAnimation() {
    currentAnimation = NONE;
    isPaused = false;
    isFadingOut = false;
}

void LedAnimations::pauseAnimation() {
    isPaused = true;
    // Current LED state is preserved
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
    currentAnimation = BREATHING;
    isFadingOut = true;
    isPaused = false;
    lastBreathUpdate = millis();
}

void LedAnimations::update() {
    if (isPaused || currentAnimation == NONE) {
        return;
    }
    
    switch (currentAnimation) {
        case BREATHING:
            updateBreathing();
            break;
        case STROBING:
            updateStrobing();
            break;
        case RAINBOW:
            updateRainbow();
            break;
        case SNAKE:
            updateSnake();
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
        currentAnimation = NONE;
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
    
    if (currentTime - lastRainbowUpdate >= rainbowStepTime) {
        lastRainbowUpdate = currentTime;
        
        // Slowly rotate the entire rainbow pattern
        rainbowHue = (rainbowHue + 1) % 256;
        
        // Define base hues for a 6-LED rainbow
        uint8_t baseHues[6] = {
            0,    // Red
            32,   // Orange
            64,   // Yellow
            96,   // Green
            160,  // Blue
            224   // Violet
        };
        
        // Set each LED to its rainbow color - adjust indices as needed for your setup
        strip1.setPixelColor(0, colorHSV((baseHues[0] + rainbowHue) % 256));
        strip1.setPixelColor(1, colorHSV((baseHues[1] + rainbowHue) % 256));
        
        strip2.setPixelColor(0, colorHSV((baseHues[2] + rainbowHue) % 256));
        strip2.setPixelColor(1, colorHSV((baseHues[3] + rainbowHue) % 256));
        strip2.setPixelColor(2, colorHSV((baseHues[4] + rainbowHue) % 256));
        strip2.setPixelColor(3, colorHSV((baseHues[5] + rainbowHue) % 256));
        
        strip1.show();
        strip2.show();
    }
}

uint32_t LedAnimations::colorHSV(uint8_t h, uint8_t s, uint8_t v) {
    // Convert HSV to RGB
    uint8_t region, remainder, p, q, t;
    uint8_t r, g, b;
    
    region = h / 43;
    remainder = (h - (region * 43)) * 6; 
    
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
        default:
            r = v; g = p; b = q;
            break;
    }
    
    return strip1.Color(r, g, b);
}

void LedAnimations::stopBreathing() {
    currentAnimation = NONE;
    rgbLed.turn_led_off();
}

void LedAnimations::setAllLeds(uint8_t red, uint8_t green, uint8_t blue) {
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
