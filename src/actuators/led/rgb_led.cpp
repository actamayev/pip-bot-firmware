#include "./rgb_led.h"

RgbLed rgbLed;

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS1, ESP_LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS2, ESP_LED_PIN2, NEO_GRB + NEO_KHZ800);

LedAnimations ledAnimations(strip1, strip2);

void RgbLed::turn_led_off() {
    set_all_leds_to_color(0, 0, 0);
}

void RgbLed::set_led_red() {
    set_all_leds_to_color(MAX_LED_BRIGHTNESS, 0, 0);
}

void RgbLed::set_led_green() {
    set_all_leds_to_color(0, MAX_LED_BRIGHTNESS, 0);
}

void RgbLed::set_led_blue() {
    set_all_leds_to_color(0, 0, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_white() {
    set_all_leds_to_color(MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_purple() {
    set_all_leds_to_color(MAX_LED_BRIGHTNESS, 0, MAX_LED_BRIGHTNESS);
}

void RgbLed::set_led_yellow() {
    set_all_leds_to_color(MAX_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS, 0);
}

void RgbLed::set_all_leds_to_color(uint8_t red, uint8_t green, uint8_t blue) {
    currentRed = red;
    currentGreen = green;
    currentBlue = blue;

    // Stop any running animations
    ledAnimations.stopAnimation();

    // Set default color for all LEDs
    for (int i = 0; i < 6; i++) {
        defaultColors[i][0] = red;
        defaultColors[i][1] = green;
        defaultColors[i][2] = blue;
        defaultColorsSet[i] = true;
    }

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

bool RgbLed::processLedUpdate(int ledIndex, uint8_t red, uint8_t green, uint8_t blue) {
    // Skip updates during rainbow animation
    if (ledAnimations.getCurrentAnimation() == LedTypes::RAINBOW) {
        return false;
    }
    
    // Store the current animation type
    LedTypes::AnimationType currentAnim = ledAnimations.getCurrentAnimation();
    
    // Update the default color for this LED
    defaultColors[ledIndex][0] = red;
    defaultColors[ledIndex][1] = green;
    defaultColors[ledIndex][2] = blue;
    defaultColorsSet[ledIndex] = true;
    
    // If we're in breathing or strobing mode, update the animation colors
    if (currentAnim == LedTypes::BREATHING) {
        ledAnimations.updateBreathingColor();
        return (ledIndex == 0); // Only return true for top-left LED, which controls breathing color
    } 
    else if (currentAnim == LedTypes::STROBING) {
        ledAnimations.updateStrobeColor();
        return (ledIndex == 0); // Only return true for top-left LED, which controls strobe color
    }
    else {
        // For other cases (including NONE), stop animations
        ledAnimations.stopAnimation();
        return true;
    }
}

void RgbLed::set_top_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(4, red, green, blue)) return;
    
    strip2.setPixelColor(2, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_top_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(3, red, green, blue)) return;
    
    strip2.setPixelColor(1, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_middle_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(5, red, green, blue)) return;
    
    strip2.setPixelColor(3, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_middle_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(2, red, green, blue)) return;
    
    strip2.setPixelColor(0, strip2.Color(red, green, blue));
    strip2.show();
}

void RgbLed::set_back_left_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(1, red, green, blue)) return;
    
    strip1.setPixelColor(1, strip1.Color(red, green, blue));
    strip1.show();
}

void RgbLed::set_back_right_led(uint8_t red, uint8_t green, uint8_t blue) {
    if (!processLedUpdate(0, red, green, blue)) return;
    
    strip1.setPixelColor(0, strip1.Color(red, green, blue));
    strip1.show();
}

void RgbLed::captureCurrentState() {
    // Capture the color of each LED
    for (int i = 0; i < 6; i++) {
        capturedState.colors[i][0] = defaultColors[i][0];
        capturedState.colors[i][1] = defaultColors[i][1];
        capturedState.colors[i][2] = defaultColors[i][2];
    }
    
    // Capture current animation state
    capturedState.animation = ledAnimations.getCurrentAnimation();
    capturedState.wasAnimationActive = (capturedState.animation != LedTypes::NONE);
    
    // Store animation speed based on type (we'll need to add getters to LedAnimations)
    // For now, we'll just store the animation type
}

void RgbLed::restoreCapturedState() {
    // First restore all LED colors
    for (int i = 0; i < 6; i++) {
        defaultColors[i][0] = capturedState.colors[i][0];
        defaultColors[i][1] = capturedState.colors[i][1];
        defaultColors[i][2] = capturedState.colors[i][2];
        defaultColorsSet[i] = true;
    }
    
    // Then restore animation if there was one
    if (capturedState.wasAnimationActive) {
        switch (capturedState.animation) {
            case LedTypes::BREATHING:
                ledAnimations.startBreathing();
                break;
            case LedTypes::STROBING:
                ledAnimations.startStrobing();
                break;
            case LedTypes::RAINBOW:
                ledAnimations.startRainbow();
                break;
            default:
                // If no animation, set all LEDs to their default colors
                set_back_right_led(capturedState.colors[0][0], capturedState.colors[0][1], capturedState.colors[0][2]);
                set_back_left_led(capturedState.colors[1][0], capturedState.colors[1][1], capturedState.colors[1][2]);
                set_middle_right_led(capturedState.colors[2][0], capturedState.colors[2][1], capturedState.colors[2][2]);
                set_top_right_led(capturedState.colors[3][0], capturedState.colors[3][1], capturedState.colors[3][2]);
                set_top_left_led(capturedState.colors[4][0], capturedState.colors[4][1], capturedState.colors[4][2]);
                set_middle_left_led(capturedState.colors[5][0], capturedState.colors[5][1], capturedState.colors[5][2]);
                break;
        }
    } else {
        // If no animation, set all LEDs to their default colors
        set_back_right_led(capturedState.colors[0][0], capturedState.colors[0][1], capturedState.colors[0][2]);
        set_back_left_led(capturedState.colors[1][0], capturedState.colors[1][1], capturedState.colors[1][2]);
        set_middle_right_led(capturedState.colors[2][0], capturedState.colors[2][1], capturedState.colors[2][2]);
        set_top_right_led(capturedState.colors[3][0], capturedState.colors[3][1], capturedState.colors[3][2]);
        set_top_left_led(capturedState.colors[4][0], capturedState.colors[4][1], capturedState.colors[4][2]);
        set_middle_left_led(capturedState.colors[5][0], capturedState.colors[5][1], capturedState.colors[5][2]);
    }
}
