#include "career_quest_triggers.h"

extern Adafruit_NeoPixel strip;
CareerQuestTriggers careerQuestTriggers(strip);

// Define static constexpr arrays
constexpr uint8_t CareerQuestTriggers::s2p1LedSequence[6];
constexpr uint8_t CareerQuestTriggers::s2p1ColorSequence[6][3];

CareerQuestTriggers::CareerQuestTriggers(Adafruit_NeoPixel& strip)
    : strip(strip) {
}

void CareerQuestTriggers::startS2P1Sequence() {
    // Turn off all LEDs first
    rgbLed.turn_all_leds_off();
    
    // Initialize sequence state
    s2p1Active = true;
    currentLedIndex = 0;
    currentColorIndex = 0;
    lastS2P1Update = millis();
    isFadingOut = false;
    currentBrightness = 0;
    targetBrightness = 255;
}

void CareerQuestTriggers::stopS2P1Sequence() {
    if (!s2p1Active) return;
    // Start exit fade instead of immediately turning off
    isExitFading = true;
    isFadingOut = true;
}

void CareerQuestTriggers::startS2P4LightShow() {
    // Turn off all LEDs first
    rgbLed.turn_all_leds_off();

    // Initialize light show state
    s2p4Active = true;
    s2p4StartTime = millis();
    lastS2P4Update = millis();
    s2p4Step = 0;
}

void CareerQuestTriggers::stopS2P4LightShow() {
    s2p4Active = false;
    
    // Turn off all LEDs
    rgbLed.turn_all_leds_off();
}

void CareerQuestTriggers::update() {
    if (s2p1Active) {
        updateS2P1Sequence();
    }
    if (s2p4Active) {
        updateS2P4LightShow();
    }
    if (s7p4Active) {
        updateS7P4ButtonDemo();
    }
}

void CareerQuestTriggers::updateS2P1Sequence() {
    unsigned long now = millis();
    
    if (now - lastS2P1Update < S2P1_UPDATE_INTERVAL) return;
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
            if (isExitFading) {
                // Exit fade complete, stop the sequence
                s2p1Active = false;
                isExitFading = false;
                isFadingOut = false;
            } else {
                // Normal fade out complete, move to next LED and start fade in
                isFadingOut = false;
                currentLedIndex = (currentLedIndex + 1) % 6;
                currentColorIndex = (currentColorIndex + 1) % 6;
                currentBrightness = 0;
                targetBrightness = 255;
            }
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

void CareerQuestTriggers::updateS2P4LightShow() {
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

void CareerQuestTriggers::startS3P3DisplayDemo() {
    s3p3Active = true;
    lastS3P3Update = millis();
    s3p3AnimationStep = 0;
}

void CareerQuestTriggers::stopS3P3DisplayDemo() {
    s3p3Active = false;
}

void CareerQuestTriggers::renderS3P3Animation() {
    if (!s3p3Active) return;
    
    unsigned long now = millis();
    if (now - lastS3P3Update >= S3P3_UPDATE_INTERVAL) {
        DisplayScreen& displayScreen = DisplayScreen::getInstance();
        Adafruit_SSD1306& display = displayScreen.display;
        
        display.clearDisplay();
        
        // Cycle through different cute animations
        switch (s3p3AnimationStep % 4) {
            case 0: {
                // Bouncing ball
                int ballX = 20 + (s3p3AnimationStep % 88);
                display.fillCircle(ballX, 32, 8, SSD1306_WHITE);
                displayScreen.drawCenteredText("Bouncing Ball!", 50, 1);
                break;
            }
            case 1: {
                // Happy face
                display.fillCircle(64, 32, 20, SSD1306_WHITE);
                display.fillCircle(56, 26, 3, SSD1306_BLACK);
                display.fillCircle(72, 26, 3, SSD1306_BLACK);
                display.drawCircle(64, 32, 8, SSD1306_BLACK);
                displayScreen.drawCenteredText("Hello!", 55, 1);
                break;
            }
            case 2: {
                // Pulsing heart
                int size = 8 + (s3p3AnimationStep % 6);
                display.fillCircle(58, 30, size, SSD1306_WHITE);
                display.fillCircle(70, 30, size, SSD1306_WHITE);
                display.fillTriangle(52, 35, 76, 35, 64, 50, SSD1306_WHITE);
                displayScreen.drawCenteredText("Love!", 55, 1);
                break;
            }
            case 3: {
                // Rotating star
                int centerX = 64, centerY = 32;
                int rotation = s3p3AnimationStep % 8;
                for (int i = 0; i < 5; i++) {
                    int angle = (i * 72 + rotation * 5) * PI / 180;
                    int x = centerX + 15 * cos(angle);
                    int y = centerY + 15 * sin(angle);
                    display.fillCircle(x, y, 2, SSD1306_WHITE);
                }
                displayScreen.drawCenteredText("Star!", 55, 1);
                break;
            }
        }
        
        s3p3AnimationStep++;
        lastS3P3Update = now;
    }
}

void CareerQuestTriggers::startS7P4ButtonDemo() {
    s7p4Active = true;
    s7p4ExitFading = false;
    s7p4CurrentBrightness = 255;
    
    // Set up button handlers for random color changes and sounds
    Buttons::getInstance().setLeftButtonClickHandler([](Button2& btn) {
        // Generate random color
        uint8_t red = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t green = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t blue = random(0, MAX_LED_BRIGHTNESS + 1);
        
        // Fade to new color using breathing animation
        rgbLed.setDefaultColors(red, green, blue);
        ledAnimations.startBreathing(1000, 0.0f); // 1s fade from dark
        
        // Play chime sound
        Speaker::getInstance().playFile(SoundType::CHIME);
    });
    
    Buttons::getInstance().setRightButtonClickHandler([](Button2& btn) {
        // Generate random color
        uint8_t red = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t green = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t blue = random(0, MAX_LED_BRIGHTNESS + 1);
        
        // Fade to new color using breathing animation
        rgbLed.setDefaultColors(red, green, blue);
        ledAnimations.startBreathing(1000, 0.0f); // 1s fade from dark
        
        // Play robot sound
        Speaker::getInstance().playFile(SoundType::ROBOT);
    });
}

void CareerQuestTriggers::stopS7P4ButtonDemo() {
    if (!s7p4Active) return;
    
    // Clear button handlers (set to nullptr)
    Buttons::getInstance().setLeftButtonClickHandler(nullptr);
    Buttons::getInstance().setRightButtonClickHandler(nullptr);
    
    // Start exit fade instead of immediately turning off
    s7p4ExitFading = true;
    s7p4CurrentBrightness = 255;
    lastS7P4Update = millis();
}

void CareerQuestTriggers::updateS7P4ButtonDemo() {
    if (!s7p4ExitFading) return;
    
    unsigned long now = millis();
    if (now - lastS7P4Update < S2P1_UPDATE_INTERVAL) return;
    
    // Fade out all main board LEDs
    if (s7p4CurrentBrightness > 0) {
        s7p4CurrentBrightness = max(0, s7p4CurrentBrightness - 5);
        
        // Apply brightness to all main board LEDs
        uint8_t fadedRed = (rgbLed.getCurrentRed() * s7p4CurrentBrightness) / 255;
        uint8_t fadedGreen = (rgbLed.getCurrentGreen() * s7p4CurrentBrightness) / 255;
        uint8_t fadedBlue = (rgbLed.getCurrentBlue() * s7p4CurrentBrightness) / 255;
        
        rgbLed.set_main_board_leds_to_color(fadedRed, fadedGreen, fadedBlue);
    } else {
        // Fade complete, stop the demo
        s7p4Active = false;
        s7p4ExitFading = false;
        rgbLed.turn_all_leds_off();
    }
    
    lastS7P4Update = now;
}
