#include "career_quest_triggers.h"
#include <algorithm>

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
    if (!s2p4Active) return;
    // Start exit fade instead of immediately turning off
    s2p4ExitFading = true;
    s2p4CurrentBrightness = 255;
}

void CareerQuestTriggers::update() {
    if (s2p1Active) updateS2P1Sequence();
    if (s2p4Active) updateS2P4LightShow();
    if (s7p4Active) updateS7P4ButtonDemo();
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
    
    // Handle exit fading
    if (s2p4ExitFading) {
        if (now - lastS2P4Update >= S2P1_UPDATE_INTERVAL) {
            // Fade out all main board LEDs
            if (s2p4CurrentBrightness > 0) {
                s2p4CurrentBrightness = std::max(0, s2p4CurrentBrightness - 5);
                
                // Get current color for fading
                uint8_t colors[][3] = {
                    {255, 0, 0},     // Red
                    {0, 255, 0},     // Green  
                    {0, 0, 255},     // Blue
                    {255, 255, 0},   // Yellow
                    {255, 0, 255},   // Magenta
                    {0, 255, 255}    // Cyan
                };
                
                uint8_t colorIndex = s2p4Step % 6;
                uint8_t red = (colors[colorIndex][0] * s2p4CurrentBrightness) / 255;
                uint8_t green = (colors[colorIndex][1] * s2p4CurrentBrightness) / 255;
                uint8_t blue = (colors[colorIndex][2] * s2p4CurrentBrightness) / 255;
                
                // Apply faded color to all main board LEDs
                for (int i = 0; i < 6; i++) {
                    uint8_t ledIndex = s2p1LedSequence[i];
                    strip.setPixelColor(ledIndex, strip.Color(red, green, blue));
                }
                strip.show();
            } else {
                // Fade complete, stop the light show
                s2p4Active = false;
                s2p4ExitFading = false;
                rgbLed.turn_all_leds_off();
            }
            lastS2P4Update = now;
        }
        return;
    }
    
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
    s3p3StartTime = millis();
    lastS3P3Update = millis();
    s3p3AnimationStep = 0;
    DisplayScreen::getInstance().displayOff = false;
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
        
        // Calculate elapsed time and loop every 10 seconds (5s HAFTR + 5s cube)
        unsigned long elapsedTime = now - s3p3StartTime;
        unsigned long cycleTime = elapsedTime % 10000; // 10 second total cycle
        
        if (cycleTime < 5000) {
            // Phase 1: HAFTR scrolling (0-5 seconds)
            float progress = (float)cycleTime / 5000.0f; // 0.0 to 1.0
            
            // Calculate text position: scroll from left (-70) to right (128+30)
            int textX = -70 + (int)(progress * 198); // Smoother scroll range
            
            display.setTextSize(3.5);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(textX, 24);
            display.print("HAFTR"); // Use print instead of println to avoid newline
        } else {
            // Phase 2: 3D rotating cube (5-10 seconds)
            unsigned long cubeTime = cycleTime - 5000;
            
            displayScreen.drawCenteredText("3D CUBE", 5, 3);
            
            // Smooth rotation based on time
            float rotX = (float)cubeTime * 0.003f;
            float rotY = (float)cubeTime * 0.004f;
            
            int centerX = 64, centerY = 35;
            int size = 12;
            
            // Calculate cube vertices with rotation
            float cosX = cos(rotX), sinX = sin(rotX);
            float cosY = cos(rotY), sinY = sin(rotY);
            
            // 3D rotation matrix application (simplified)
            // Front face vertices
            float fx1 = size, fy1 = size, fz1 = size;
            float fx2 = -size, fy2 = size, fz2 = size;
            float fx3 = -size, fy3 = -size, fz3 = size;
            float fx4 = size, fy4 = -size, fz4 = size;
            
            // Apply rotation and project to 2D
            int x1 = centerX + (int)(fx1 * cosY + fz1 * sinY);
            int y1 = centerY + (int)(fy1 * cosX);
            int x2 = centerX + (int)(fx2 * cosY + fz2 * sinY);
            int y2 = centerY + (int)(fy2 * cosX);
            int x3 = centerX + (int)(fx3 * cosY + fz3 * sinY);
            int y3 = centerY + (int)(fy3 * cosX);
            int x4 = centerX + (int)(fx4 * cosY + fz4 * sinY);
            int y4 = centerY + (int)(fy4 * cosX);
            
            // Back face vertices (z = -size)
            float bx1 = size, by1 = size, bz1 = -size;
            float bx2 = -size, by2 = size, bz2 = -size;
            float bx3 = -size, by3 = -size, bz3 = -size;
            float bx4 = size, by4 = -size, bz4 = -size;
            
            int bx1_2d = centerX + (int)(bx1 * cosY + bz1 * sinY);
            int by1_2d = centerY + (int)(by1 * cosX);
            int bx2_2d = centerX + (int)(bx2 * cosY + bz2 * sinY);
            int by2_2d = centerY + (int)(by2 * cosX);
            int bx3_2d = centerX + (int)(bx3 * cosY + bz3 * sinY);
            int by3_2d = centerY + (int)(by3 * cosX);
            int bx4_2d = centerX + (int)(bx4 * cosY + bz4 * sinY);
            int by4_2d = centerY + (int)(by4 * cosX);
            
            // Draw front face
            display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
            display.drawLine(x2, y2, x3, y3, SSD1306_WHITE);
            display.drawLine(x3, y3, x4, y4, SSD1306_WHITE);
            display.drawLine(x4, y4, x1, y1, SSD1306_WHITE);
            
            // Draw back face
            display.drawLine(bx1_2d, by1_2d, bx2_2d, by2_2d, SSD1306_WHITE);
            display.drawLine(bx2_2d, by2_2d, bx3_2d, by3_2d, SSD1306_WHITE);
            display.drawLine(bx3_2d, by3_2d, bx4_2d, by4_2d, SSD1306_WHITE);
            display.drawLine(bx4_2d, by4_2d, bx1_2d, by1_2d, SSD1306_WHITE);
            
            // Connect front and back faces
            display.drawLine(x1, y1, bx1_2d, by1_2d, SSD1306_WHITE);
            display.drawLine(x2, y2, bx2_2d, by2_2d, SSD1306_WHITE);
            display.drawLine(x3, y3, bx3_2d, by3_2d, SSD1306_WHITE);
            display.drawLine(x4, y4, bx4_2d, by4_2d, SSD1306_WHITE);
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

void CareerQuestTriggers::stopAllCareerQuestTriggers() {
    // Stop all active career quest triggers (these will start fade sequences)
    stopS2P1Sequence();
    stopS2P4LightShow();
    stopS3P3DisplayDemo();
    stopS7P4ButtonDemo();
    
    // Stop all LED animations (but don't force LEDs off - let career quest fades complete)
    ledAnimations.stopAnimation();
    ledAnimations.fadeOut();
    
    // Stop all audio
    Speaker::getInstance().stopAllSounds();
    
    // Disable all sensor data sending
    SendSensorData::getInstance().setSendSensorData(false);
    SendSensorData::getInstance().setSendMultizoneData(false);
    SendSensorData::getInstance().setEulerDataEnabled(false);
    SendSensorData::getInstance().setAccelDataEnabled(false);
    SendSensorData::getInstance().setGyroDataEnabled(false);
    SendSensorData::getInstance().setMagnetometerDataEnabled(false);
    SendSensorData::getInstance().setMultizoneTofDataEnabled(false);
    SendSensorData::getInstance().setSideTofDataEnabled(false);
    SendSensorData::getInstance().setColorSensorDataEnabled(false);
    SendSensorData::getInstance().setEncoderDataEnabled(false);
    
    // Stop motors
    motorDriver.stop_both_motors();
}
