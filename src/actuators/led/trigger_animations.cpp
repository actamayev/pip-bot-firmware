#include "trigger_animations.h"

extern Adafruit_NeoPixel strip;
TriggerAnimations triggerAnimations(strip);

// Define static constexpr arrays
constexpr uint8_t TriggerAnimations::s2p1LedSequence[6];
constexpr uint8_t TriggerAnimations::s2p1ColorSequence[6][3];

TriggerAnimations::TriggerAnimations(Adafruit_NeoPixel& strip)
    : strip(strip) {
}

void TriggerAnimations::startS2P1Sequence() {
    // Turn off all LEDs first
    for(int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
    
    // Initialize sequence state
    s2p1Active = true;
    currentLedIndex = 0;
    currentColorIndex = 0;
    lastS2P1Update = millis();
    isFadingOut = false;
    currentBrightness = 0;
    targetBrightness = 255;
}

void TriggerAnimations::stopS2P1Sequence() {
    s2p1Active = false;
    
    // Turn off all LEDs
    for(int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
}

void TriggerAnimations::startS2P4LightShow() {
    // Turn off all LEDs first
    for(int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
    
    // Initialize light show state
    s2p4Active = true;
    s2p4StartTime = millis();
    lastS2P4Update = millis();
    s2p4Step = 0;
}

void TriggerAnimations::stopS2P4LightShow() {
    s2p4Active = false;
    
    // Turn off all LEDs
    for(int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
}

void TriggerAnimations::update() {
    if (s2p1Active) {
        updateS2P1Sequence();
    }
    if (s2p4Active) {
        updateS2P4LightShow();
    }
}

void TriggerAnimations::updateS2P1Sequence() {
    unsigned long now = millis();
    
    if (now - lastS2P1Update >= S2P1_UPDATE_INTERVAL) {
        uint8_t currentLed = s2p1LedSequence[currentLedIndex];
        uint8_t red = s2p1ColorSequence[currentColorIndex][0];
        uint8_t green = s2p1ColorSequence[currentColorIndex][1];
        uint8_t blue = s2p1ColorSequence[currentColorIndex][2];
        
        if (isFadingOut) {
            // Fade out current LED
            if (currentBrightness > 0) {
                currentBrightness = max(0, currentBrightness - 5); // Fade out step
                
                // Apply brightness to current LED
                uint8_t fadedRed = (red * currentBrightness) / 255;
                uint8_t fadedGreen = (green * currentBrightness) / 255;
                uint8_t fadedBlue = (blue * currentBrightness) / 255;
                
                strip.setPixelColor(currentLed, strip.Color(fadedRed, fadedGreen, fadedBlue));
                strip.show();
            } else {
                // Fade out complete, move to next LED and start fade in
                isFadingOut = false;
                currentLedIndex = (currentLedIndex + 1) % 6;
                currentColorIndex = (currentColorIndex + 1) % 6;
                currentBrightness = 0;
                targetBrightness = 255;
            }
        } else {
            // Fade in new LED
            if (currentBrightness < targetBrightness) {
                currentBrightness = min(255, currentBrightness + 5); // Fade in step
                
                // Apply brightness to new LED with new color
                red = s2p1ColorSequence[currentColorIndex][0];
                green = s2p1ColorSequence[currentColorIndex][1];
                blue = s2p1ColorSequence[currentColorIndex][2];
                
                uint8_t fadedRed = (red * currentBrightness) / 255;
                uint8_t fadedGreen = (green * currentBrightness) / 255;
                uint8_t fadedBlue = (blue * currentBrightness) / 255;
                
                currentLed = s2p1LedSequence[currentLedIndex];
                strip.setPixelColor(currentLed, strip.Color(fadedRed, fadedGreen, fadedBlue));
                strip.show();
            } else {
                // Fade in complete, start fade out
                isFadingOut = true;
            }
        }
        
        lastS2P1Update = now;
    }
}

void TriggerAnimations::updateS2P4LightShow() {
    unsigned long now = millis();
    
    // Check if 5 seconds have elapsed
    if (now - s2p4StartTime >= S2P4_DURATION) {
        stopS2P4LightShow();
        return;
    }
    
    if (now - lastS2P4Update >= S2P4_UPDATE_INTERVAL) {
        // Simple light show: alternating colors on all LEDs
        uint8_t colors[][3] = {
            {255, 0, 0},     // Red
            {0, 255, 0},     // Green  
            {0, 0, 255},     // Blue
            {255, 255, 0},   // Yellow
            {255, 0, 255},   // Magenta
            {0, 255, 255}    // Cyan
        };
        
        uint8_t colorIndex = s2p4Step % 6;
        uint8_t red = colors[colorIndex][0];
        uint8_t green = colors[colorIndex][1];
        uint8_t blue = colors[colorIndex][2];
        
        // Set all main board LEDs to current color (skip headlights)
        for (int i = 0; i < 6; i++) {
            uint8_t ledIndex = s2p1LedSequence[i];
            strip.setPixelColor(ledIndex, strip.Color(red, green, blue));
        }
        strip.show();
        
        s2p4Step++;
        lastS2P4Update = now;
    }
}