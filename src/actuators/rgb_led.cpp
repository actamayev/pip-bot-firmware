#include "./rgb_led.h"

RgbLed rgbLed;

// Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, ESP_LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS1, ESP_LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS2, ESP_LED_PIN2, NEO_GRB + NEO_KHZ800);

void RgbLed::turn_led_off() {
    set_led_to_color(0, 0, 0);
}

void RgbLed::set_led_red() {
    set_led_to_color(255, 0, 0);
}

void RgbLed::set_led_green() {
    set_led_to_color(0, 255, 0);
}

void RgbLed::set_led_blue() {
    set_led_to_color(0, 0, 255);
}

void RgbLed::set_led_white() {
    set_led_to_color(255, 255, 255);
}

void RgbLed::set_led_purple() {
    set_led_to_color(255, 0, 255);
}

void RgbLed::set_led_to_color(uint8_t red, uint8_t green, uint8_t blue) {
    // strip.setPixelColor(0, strip.Color(red, green, blue));
    // stripe.show();

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

void RgbLed::update() {
    if (!isBreathing) return;
    
    unsigned long currentTime = millis();
    
    // Calculate the time for each step based on the total cycle time
    // We want to complete a full sine wave (0 to 2π) in the given time
    // breatheSpeed is now the time for a full min-to-max transition in milliseconds
    unsigned long timePerStep = breatheSpeed / 255;
    
    // Ensure we have at least 1ms per step
    timePerStep = max(1UL, timePerStep);
    
    // Only update if enough time has passed
    if (currentTime - lastBreathUpdate < timePerStep) return;
    lastBreathUpdate = currentTime;
    
    // Calculate current color based on breath progress
    float factor;
    // We'll use a sine wave for smooth transitions in both directions
    factor = (sin(breathProgress * PI) + 1.0) / 2.0;
    
    // Interpolate between min and max for each color
    uint8_t r = breathMin[0] + factor * (breathMax[0] - breathMin[0]);
    uint8_t g = breathMin[1] + factor * (breathMax[1] - breathMin[1]);
    uint8_t b = breathMin[2] + factor * (breathMax[2] - breathMin[2]);
    
    // Set the color
    set_led_to_color(r, g, b);
    
    // Update progress - we're now using a continuous cycle
    float progressStep = 1.0 / 255.0;
    breathProgress += progressStep;
    
    // Reset progress when we complete a full cycle
    if (breathProgress >= 2.0) {
        breathProgress = 0.0;
    }
}

// Modified start breathing function with min/max ranges
void RgbLed::startBreathing(uint8_t redMin, uint8_t redMax, 
                           uint8_t greenMin, uint8_t greenMax, 
                           uint8_t blueMin, uint8_t blueMax, 
                           int speed) {
    breathMin[0] = redMin;
    breathMin[1] = greenMin;
    breathMin[2] = blueMin;

    breathMax[0] = redMax;
    breathMax[1] = greenMax;
    breathMax[2] = blueMax;
   
    // speed is now the total time for a min-to-max transition in milliseconds
    breatheSpeed = speed;
    breathProgress = 0.0;
    // breatheDirection is no longer needed but keeping it for compatibility
    breatheDirection = true;
    isBreathing = true;
}

void RgbLed::stopBreathing() {
    isBreathing = false;
    turn_led_off();
}
