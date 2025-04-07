#include "./rgb_led.h"

RgbLed rgbLed;

// Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, ESP_LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS1, ESP_LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS2, ESP_LED_PIN2, NEO_GRB + NEO_KHZ800);

void RgbLed::turn_led_off() {
    set_all_leds_to_color(0, 0, 0);
}

void RgbLed::set_led_red() {
    set_all_leds_to_color(255, 0, 0);
}

void RgbLed::set_led_green() {
    set_all_leds_to_color(0, 255, 0);
}

void RgbLed::set_led_blue() {
    set_all_leds_to_color(0, 0, 255);
}

void RgbLed::set_led_white() {
    set_all_leds_to_color(255, 255, 255);
}

void RgbLed::set_led_purple() {
    set_all_leds_to_color(255, 0, 255);
}

void RgbLed::startStrobe(uint8_t red, uint8_t green, uint8_t blue, int speed) {
    // Stop other animations
    isBreathing = false;
    isRainbow = false;
    isFadingOut = false;
    
    // Set up strobe parameters
    strobeColor[0] = red;
    strobeColor[1] = green;
    strobeColor[2] = blue;
    strobeSpeed = speed;
    strobeState = false;
    isStrobing = true;
    lastStrobeUpdate = millis();
}

void RgbLed::startRainbow(int speed) {
    // Stop other animations
    isBreathing = false;
    isStrobing = false;
    isFadingOut = false;
    
    // Set up rainbow parameters
    rainbowSpeed = speed;
    rainbowHue = 0;
    isRainbow = true;
    lastRainbowUpdate = millis();
}

// Modify the update method to include strobe and rainbow animations
void RgbLed::update() {
    unsigned long currentTime = millis();
    
    // Handle breathing animation
    if (isBreathing) {
        unsigned long timePerStep = breatheSpeed / 255;
        timePerStep = max(1UL, timePerStep);
        
        if (currentTime - lastBreathUpdate < timePerStep) return;
        lastBreathUpdate = currentTime;
        
        float factor;
        
        // Calculate the breathing factor
        factor = (sin(breathProgress * PI) + 1.0) / 2.0;
        
        // Interpolate between min and max for each color
        uint8_t r = breathMin[0] + factor * (breathMax[0] - breathMin[0]);
        uint8_t g = breathMin[1] + factor * (breathMax[1] - breathMin[1]);
        uint8_t b = breathMin[2] + factor * (breathMax[2] - breathMin[2]);
        
        // Set the color
        set_all_leds_to_color(r, g, b);
        
        // Update progress
        float progressStep = 1.0 / 255.0;
        breathProgress += progressStep;
        
        // Check if we're fading out and have reached the bottom of the cycle
        if (isFadingOut && breathProgress >= 1.0) {
            // We've faded out completely, stop breathing and ensure LEDs are off
            isBreathing = false;
            isFadingOut = false;
            set_all_leds_to_color(0, 0, 0);
            return;
        }
        
        // For normal breathing, reset when we complete a full cycle
        if (breathProgress >= 2.0) {
            breathProgress = 0.0;
        }
    }
    
    // Handle strobe animation
    if (isStrobing) {
        if (currentTime - lastStrobeUpdate >= strobeSpeed) {
            lastStrobeUpdate = currentTime;
            strobeState = !strobeState;
            
            if (strobeState) {
                // ON state - use the set color
                set_all_leds_to_color(strobeColor[0], strobeColor[1], strobeColor[2]);
            } else {
                // OFF state
                set_all_leds_to_color(0, 0, 0);
            }
        }
    }
    
    // Handle rainbow animation
    if (isRainbow) {
        if (currentTime - lastRainbowUpdate >= rainbowSpeed) {
            lastRainbowUpdate = currentTime;
            
            // Increment hue to cycle through the rainbow
            rainbowHue = (rainbowHue + 1) % 256;
            
            // Convert HSV to RGB - simplified rainbow effect
            // Based on FastLED's rainbow algorithm
            for (int i = 0; i < NUM_LEDS1; i++) {
                int pixelHue = (rainbowHue + (i * 256 / NUM_LEDS1)) % 256;
                strip1.setPixelColor(i, colorHSV(pixelHue));
            }
            
            for (int i = 0; i < NUM_LEDS2; i++) {
                int pixelHue = (rainbowHue + (i * 256 / NUM_LEDS2)) % 256;
                strip2.setPixelColor(i, colorHSV(pixelHue));
            }
            
            strip1.show();
            strip2.show();
        }
    }
}
// Helper function to convert HSV to RGB
// h: 0-255, s: 0-255, v: 0-255
// Implementation with default parameters (only one function needed)
uint32_t RgbLed::colorHSV(uint8_t h, uint8_t s, uint8_t v) {
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

void RgbLed::startBreathing(uint8_t redMin, uint8_t redMax, 
    uint8_t greenMin, uint8_t greenMax, 
    uint8_t blueMin, uint8_t blueMax, 
    int speed
) {
    // Check if top-left LED has a default color set
    if (defaultColorsSet[0]) {
        breathMin[0] = 0.1 * defaultColors[0][0]; // starts at 10% brightness
        breathMin[1] = 0.1 * defaultColors[0][1];
        breathMin[2] = 0.1 * defaultColors[0][2];

        // Use top-left LED color as max values
        breathMax[0] = defaultColors[0][0];
        breathMax[1] = defaultColors[0][1];
        breathMax[2] = defaultColors[0][2];
    } else {
        // Fall back to provided parameters
        breathMin[0] = redMin;
        breathMin[1] = greenMin;
        breathMin[2] = blueMin;

        breathMax[0] = redMax;
        breathMax[1] = greenMax;
        breathMax[2] = blueMax;
    }

    breatheSpeed = speed;
    breathProgress = 0.0;
    breatheDirection = true;
    isBreathing = true;
}

// Update the set_all_leds_to_color function to handle default colors
void RgbLed::set_all_leds_to_color(uint8_t red, uint8_t green, uint8_t blue) {
    currentRed = red;
    currentGreen = green;
    currentBlue = blue;

    // Only update default colors when not in an animation
    if (!isBreathing && !isRainbow && !isStrobing && !isFadingOut) {
        // Set default color for all LEDs
        for (int i = 0; i < 6; i++) {
            defaultColors[i][0] = red;
            defaultColors[i][1] = green;
            defaultColors[i][2] = blue;
            defaultColorsSet[i] = true;
        }
    }

    // Set all LEDs to the same color
    for(int i = 0; i < NUM_LEDS1; i++) {
        strip1.setPixelColor(i, strip1.Color(red, green, blue));
    }
    for(int i = 0; i < NUM_LEDS2; i++) {
        strip2.setPixelColor(i, strip2.Color(red, green, blue));
    }
    strip1.show();
    strip2.show();
}

void RgbLed::stopBreathing() {
    isBreathing = false;
    turn_led_off();
}

void RgbLed::pauseBreathing() {
    isBreathing = false;
    // Note: We don't turn off the LED, just stop updating it
    // Current color values are preserved
}

// TODO: This doesn't work correctly - turns off too quickly.
void RgbLed::fadeOut() {
    // Set min values to 0 and max values to current
    breathMin[0] = 0;
    breathMin[1] = 0;
    breathMin[2] = 0;
    
    breathMax[0] = currentRed;
    breathMax[1] = currentGreen;
    breathMax[2] = currentBlue;
    
    // Start at the "top" of the breath cycle
    breathProgress = 0.0;
    
    // Start breathing to fade out
    isBreathing = true;
    
    // Add the isFadingOut flag as suggested in previous solution
    isFadingOut = true;
}

// Implementation of individual LED control methods
void RgbLed::set_top_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    // If we're not in an animation, store the default color for this LED
    if (!isBreathing && !isRainbow && !isStrobing && !isFadingOut) {
        defaultColors[0][0] = red;
        defaultColors[0][1] = green;
        defaultColors[0][2] = blue;
        defaultColorsSet[0] = true;
    }
    
    strip1.setPixelColor(0, strip1.Color(red, green, blue));
    strip1.show();
}

void RgbLed::set_top_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    // If we're not in an animation, store the default color for this LED
    if (!isBreathing && !isRainbow && !isStrobing && !isFadingOut) {
        defaultColors[1][0] = red;
        defaultColors[1][1] = green;
        defaultColors[1][2] = blue;
        defaultColorsSet[1] = true;
    }
    
    strip1.setPixelColor(1, strip1.Color(red, green, blue));
    strip1.show();
}

void RgbLed::set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    // If we're not in an animation, store the default color for this LED
    if (!isBreathing && !isRainbow && !isStrobing && !isFadingOut) {
        defaultColors[2][0] = red;
        defaultColors[2][1] = green;
        defaultColors[2][2] = blue;
        defaultColorsSet[2] = true;
    }
    
    strip2.setPixelColor(0, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    // If we're not in an animation, store the default color for this LED
    if (!isBreathing && !isRainbow && !isStrobing && !isFadingOut) {
        defaultColors[3][0] = red;
        defaultColors[3][1] = green;
        defaultColors[3][2] = blue;
        defaultColorsSet[3] = true;
    }
    
    strip2.setPixelColor(1, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_back_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    // If we're not in an animation, store the default color for this LED
    if (!isBreathing && !isRainbow && !isStrobing && !isFadingOut) {
        defaultColors[4][0] = red;
        defaultColors[4][1] = green;
        defaultColors[4][2] = blue;
        defaultColorsSet[4] = true;
    }
    
    strip2.setPixelColor(2, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_back_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    // If we're not in an animation, store the default color for this LED
    if (!isBreathing && !isRainbow && !isStrobing && !isFadingOut) {
        defaultColors[5][0] = red;
        defaultColors[5][1] = green;
        defaultColors[5][2] = blue;
        defaultColorsSet[5] = true;
    }
    
    strip2.setPixelColor(3, strip2.Color(red, green, blue));
    strip2.show();
}

// Updated stopAllAnimations method to restore all LED defaults
void RgbLed::stopAllAnimations() {
    isBreathing = false;
    isStrobing = false;
    isRainbow = false;
    isFadingOut = false;
    
    // Restore each LED to its default color if set
    if (defaultColorsSet[0]) set_top_left_led(defaultColors[0][0], defaultColors[0][1], defaultColors[0][2]);
    if (defaultColorsSet[1]) set_top_right_led(defaultColors[1][0], defaultColors[1][1], defaultColors[1][2]);
    if (defaultColorsSet[2]) set_middle_left_led(defaultColors[2][0], defaultColors[2][1], defaultColors[2][2]);
    if (defaultColorsSet[3]) set_middle_right_led(defaultColors[3][0], defaultColors[3][1], defaultColors[3][2]);
    if (defaultColorsSet[4]) set_back_left_led(defaultColors[4][0], defaultColors[4][1], defaultColors[4][2]);
    if (defaultColorsSet[5]) set_back_right_led(defaultColors[5][0], defaultColors[5][1], defaultColors[5][2]);
}
