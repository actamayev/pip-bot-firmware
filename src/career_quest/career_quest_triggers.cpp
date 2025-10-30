#include "career_quest_triggers.h"
#include "sensors/sensor_data_buffer.h"
#include <algorithm>

extern Adafruit_NeoPixel strip;
CareerQuestTriggers careerQuestTriggers(strip);

// Define static constexpr arrays
constexpr uint8_t CareerQuestTriggers::s2p1LedSequence[8];
constexpr uint8_t CareerQuestTriggers::s2p1ColorSequence[8][3];
constexpr unsigned long CareerQuestTriggers::S5P4_UPDATE_INTERVAL;
constexpr unsigned long CareerQuestTriggers::S2P4_DURATION;
constexpr unsigned long CareerQuestTriggers::S2P4_SNAKE_DURATION;
constexpr unsigned long CareerQuestTriggers::S2P4_FLASH_DURATION;
constexpr unsigned long CareerQuestTriggers::S2P4_BREATHING_DURATION;
constexpr unsigned long CareerQuestTriggers::S2P4_SNAKE_INTERVAL;
constexpr unsigned long CareerQuestTriggers::S2P4_FLASH_INTERVAL;

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
    // Stop any existing LED animations
    ledAnimations.stopAnimation();

    // Initialize light show state
    s2p4Active = true;
    s2p4StartTime = millis();
    lastS2P4Update = millis();
    s2p4Step = 0;
    s2p4Phase = 0; // Start with snake
    s2p4SnakePosition = 0;
    s2p4FlashColorIndex = 0;
    s2p4SnakeDirection = true;
    s2p4PhaseStartTime = millis();
}

void CareerQuestTriggers::stopS2P4LightShow() {
    if (!s2p4Active) return;
    // Stop any breathing animation immediately
    ledAnimations.stopAnimation();
    // Start exit fade instead of immediately turning off
    s2p4ExitFading = true;
    s2p4CurrentBrightness = 255;
}

void CareerQuestTriggers::update() {
    if (s2p1Active) updateS2P1Sequence();
    if (s2p4Active) updateS2P4LightShow();
    if (s7p4Active) updateS7P4ButtonDemo();
    if (s5p4Active) updateS5P4LedVisualization();
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
            currentBrightness = max(0, currentBrightness - S2P1_BRIGHTNESS_STEP); // Fade out step
            
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
                currentLedIndex = (currentLedIndex + 1) % 8;
                currentColorIndex = (currentColorIndex + 1) % 8;
                currentBrightness = 0;
                targetBrightness = 255;
            }
        }
    } else {
        // Fade in new LED
        if (currentBrightness < targetBrightness) {
            currentBrightness = min(255, currentBrightness + S2P1_BRIGHTNESS_STEP); // Fade in step
            
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
        // Stop any breathing animation immediately and turn off all LEDs
        ledAnimations.stopAnimation();
        rgbLed.turn_all_leds_off();
        s2p4Active = false;
        s2p4ExitFading = false;
        return;
    }
    
    // Check total duration
    if (now - s2p4StartTime >= S2P4_DURATION) {
        stopS2P4LightShow();
        return;
    }
    
    unsigned long phaseElapsed = now - s2p4PhaseStartTime;
    
    // Phase transitions
    if (s2p4Phase == 0 && phaseElapsed >= S2P4_SNAKE_DURATION) {
        // Transition to flash phase
        s2p4Phase = 1;
        s2p4PhaseStartTime = now;
        s2p4FlashColorIndex = 0;
        rgbLed.turn_all_leds_off();
        return;
    } else if (s2p4Phase == 1 && phaseElapsed >= S2P4_FLASH_DURATION) {
        // Transition to breathing phase
        s2p4Phase = 2;
        s2p4PhaseStartTime = now;
        rgbLed.turn_all_leds_off();
        // Start breathing animation with first S2P1 color (red)
        uint8_t r = s2p1ColorSequence[0][0];
        uint8_t g = s2p1ColorSequence[0][1];
        uint8_t b = s2p1ColorSequence[0][2];
        rgbLed.setDefaultColors(r, g, b);
        ledAnimations.startBreathing(2000, 0.0f); // 2s breathing cycle
        return;
    }
    
    // Phase 0: Snake animation
    if (s2p4Phase == 0) {
        if (now - lastS2P4Update >= S2P4_SNAKE_INTERVAL) {
            rgbLed.turn_all_leds_off();
            
            // Use S2P1 color sequence for snake
            uint8_t colorIndex = s2p4SnakePosition % 8;
            uint8_t r = s2p1ColorSequence[colorIndex][0];
            uint8_t g = s2p1ColorSequence[colorIndex][1];
            uint8_t b = s2p1ColorSequence[colorIndex][2];
            
            // Light up current LED in snake sequence
            uint8_t currentLed = s2p1LedSequence[s2p4SnakePosition];
            strip.setPixelColor(currentLed, strip.Color(r, g, b));
            strip.show();
            
            // Advance snake position
            if (s2p4SnakeDirection) {
                s2p4SnakePosition++;
                if (s2p4SnakePosition >= 8) {
                    s2p4SnakePosition = 6; // Start going back
                    s2p4SnakeDirection = false;
                }
            } else {
                if (s2p4SnakePosition > 0) {
                    s2p4SnakePosition--;
                } else {
                    s2p4SnakeDirection = true;
                    s2p4SnakePosition = 1; // Start going forward again
                }
            }
            
            lastS2P4Update = now;
        }
    }
    // Phase 1: Flash all LEDs with ROY G BIV colors
    else if (s2p4Phase == 1) {
        if (now - lastS2P4Update >= S2P4_FLASH_INTERVAL) {
            // ROY G BIV color sequence
            uint8_t roygbivColors[][3] = {
                {255, 0, 0},     // Red
                {255, 127, 0},   // Orange
                {255, 255, 0},   // Yellow
                {0, 255, 0},     // Green
                {0, 0, 255},     // Blue
                {75, 0, 130},    // Indigo
                {148, 0, 211}    // Violet
            };
            
            uint8_t colorIndex = s2p4FlashColorIndex % 7;
            uint8_t red = roygbivColors[colorIndex][0];
            uint8_t green = roygbivColors[colorIndex][1];
            uint8_t blue = roygbivColors[colorIndex][2];
            
            // Flash all LEDs with current color
            for (int i = 0; i < 8; i++) {
                uint8_t ledIndex = s2p1LedSequence[i];
                strip.setPixelColor(ledIndex, strip.Color(red, green, blue));
            }
            strip.show();
            
            s2p4FlashColorIndex++;
            lastS2P4Update = now;
        }
    }
    // Phase 2: Breathing animation (handled by LED animations system)
    // No update needed here - ledAnimations.update() handles it
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
        
        // Calculate elapsed time and loop:
        // Phase1: 0-5s  -> HAFTR scroll
        // Phase2: 5-10s -> 3D cube
        // Phase3: 10-15s -> speed gauge (5s: 2.5s up, 2.5s down)
        unsigned long elapsedTime = now - s3p3StartTime;
        const unsigned long PH1 = 5000UL;
        const unsigned long PH2 = 5000UL;
        const unsigned long PH3 = 5000UL;
        const unsigned long TOTAL = PH1 + PH2 + PH3; // 15000 ms
        unsigned long cycleTime = elapsedTime % TOTAL;
        
        if (cycleTime < PH1) {
            // Phase 1: HAFTR scrolling (0-5 seconds)
            float progress = (float)cycleTime / (float)PH1; // 0.0 .. 1.0

            display.setTextSize(3.5);
            display.setTextColor(SSD1306_WHITE);

            // Compute text width so scroll range adapts to word size
            int16_t xb, yb;
            uint16_t tw, th;
            const char* word = "HAFTR";
            display.getTextBounds(word, 0, 0, &xb, &yb, &tw, &th);

            int startX = - (int)tw;                 // start fully off-left
            int endX   = SCREEN_WIDTH + 30;         // end past the right edge
            int textX  = startX + (int)(progress * (endX - startX));
            int textY  = 24;

            display.setTextWrap(false);             // prevent automatic wrapping
            display.setCursor(textX, textY);
            display.print(word);
            display.setTextWrap(true);              // restore wrap
        }
        else if (cycleTime < (PH1 + PH2)) {
            // Phase 2: 3D rotating cube (5-10 seconds) — half the previous rotation speed
            unsigned long t = cycleTime - PH1;     // 0..PH2-1

            // half-speed rotations (previously 0.003f / 0.004f / 0.0025f)
            float ax = (float)t * 0.0015f;
            float ay = (float)t * 0.0020f;
            float az = (float)t * 0.00125f;

            const int centerX = 64;
            const int centerY = 35;
            const float SIZE_PIX = 12.0f;   // cube half-size in model units (tweak to scale)
            const float FX = 95.0f;         // focal length
            const float ZCAM = 60.0f;       // camera distance

            // Cube verts and edges
            struct Vec3 { float x, y, z; };
            static const Vec3 CUBE[8] = {
                {-1,-1,-1}, {+1,-1,-1}, {+1,+1,-1}, {-1,+1,-1},
                {-1,-1,+1}, {+1,-1,+1}, {+1,+1,+1}, {-1,+1,+1}
            };
            static const uint8_t EDGES[12][2] = {
                {0,1},{1,2},{2,3},{3,0},
                {4,5},{5,6},{6,7},{7,4},
                {0,4},{1,5},{2,6},{3,7}
            };

            // Precompute sin/cos
            float sx = sin(ax), cxr = cos(ax);
            float sy = sin(ay), cyr = cos(ay);
            float sz = sin(az), czr = cos(az);

            float px[8], py[8];
            for (uint8_t i = 0; i < 8; i++) {
                float x = CUBE[i].x * SIZE_PIX;
                float y = CUBE[i].y * SIZE_PIX;
                float z = CUBE[i].z * SIZE_PIX;

                // Rotate X
                float y1 = y * cxr - z * sx;
                float z1 = y * sx  + z * cxr;
                float x1 = x;

                // Rotate Y
                float x2 = x1 * cyr + z1 * sy;
                float z2 = -x1 * sy + z1 * cyr;
                float y2 = y1;

                // Rotate Z
                float x3 = x2 * czr - y2 * sz;
                float y3 = x2 * sz  + y2 * czr;
                float z3 = z2;

                // Project
                float denom = (z3 + ZCAM);
                if (denom <= 1.0f) denom = 1.0f;
                float s = FX / denom;

                px[i] = centerX + x3 * s;
                py[i] = centerY + y3 * s;
            }

            // Draw edges
            for (uint8_t e = 0; e < 12; e++) {
                uint8_t i0 = EDGES[e][0], i1 = EDGES[e][1];
                display.drawLine((int16_t)px[i0], (int16_t)py[i0],
                                 (int16_t)px[i1], (int16_t)py[i1], SSD1306_WHITE);
            }
        }
        else {
            // Phase 3: Speed gauge (10-15s) -> 5 seconds total: 2.5s up, 2.5s down
            unsigned long t = cycleTime - (PH1 + PH2); // 0 .. PH3-1
            const unsigned long HALF = PH3 / 2;        // 2500 ms
            float progress;
            if (t < HALF) {
                progress = (float)t / (float)HALF;     // 0..1 (rev up)
            } else {
                progress = 1.0f - (float)(t - HALF) / (float)HALF; // 1..0 (rev down)
            }

            // Gauge geometry
            const int cx = 64;
            const int cy = 48;
            const int radius = 22;
            const float START_ANGLE = -3.0f * PI / 4.0f; // -135 degrees
            const float END_ANGLE   =  3.0f * PI / 4.0f; // +135 degrees
            const float sweep = END_ANGLE - START_ANGLE;

            // Draw outer arc (approximate by plotting pixels)
            for (float a = START_ANGLE; a <= END_ANGLE; a += 0.04f) {
                int x = cx + (int)(cos(a) * radius);
                int y = cy + (int)(sin(a) * radius);
                display.drawPixel(x, y, SSD1306_WHITE);
            }

            // Draw ticks (major ticks every 15 degrees)
            const int NUM_TICKS = 11;
            for (int i = 0; i < NUM_TICKS; ++i) {
                float ta = START_ANGLE + (sweep * (float)i / (NUM_TICKS - 1));
                int x1 = cx + (int)(cos(ta) * (radius - 1));
                int y1 = cy + (int)(sin(ta) * (radius - 1));
                int x2 = cx + (int)(cos(ta) * (radius - 5));
                int y2 = cy + (int)(sin(ta) * (radius - 5));
                display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
            }

            // Draw filled progress arc (draw small radial points from start to current angle)
            float curAngle = START_ANGLE + progress * sweep;
            for (float a = START_ANGLE; a <= curAngle; a += 0.03f) {
                int x = cx + (int)(cos(a) * (radius - 2));
                int y = cy + (int)(sin(a) * (radius - 2));
                display.drawPixel(x, y, SSD1306_WHITE);
            }

            // Draw center pivot
            display.fillCircle(cx, cy, 2, SSD1306_WHITE);

            // Draw needle
            int nx = cx + (int)(cos(curAngle) * (radius - 6));
            int ny = cy + (int)(sin(curAngle) * (radius - 6));
            display.drawLine(cx, cy, nx, ny, SSD1306_WHITE);
            // small extra line to make needle thicker
            display.drawLine(cx+1, cy, nx+1, ny, SSD1306_WHITE);

            // Draw numeric speed (0-100)
            int speedValue = (int)(progress * 100.0f + 0.5f);
            char buf[6];
            snprintf(buf, sizeof(buf), "%3d", speedValue);
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            int16_t tbx, tby; uint16_t tbw, tbh;
            display.getTextBounds(buf, 0, 0, &tbx, &tby, &tbw, &tbh);
            // position the number above the gauge center
            display.setCursor(cx - tbw / 2, cy - radius - 12);
            display.print(buf);

            // Label
            display.setTextSize(1);
            display.setCursor(cx - 16, cy + 8);
            display.print("SPEED");
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
        Speaker::getInstance().playFile(SoundType::FART);
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

void CareerQuestTriggers::startS5P4LedVisualization() {
    rgbLed.turn_all_leds_off();
    s5p4Active = true;
    s5p4ExitFading = false;
    lastS5P4Update = millis();
}

void CareerQuestTriggers::stopS5P4LedVisualization() {
    if (!s5p4Active) return;
    s5p4ExitFading = true;
    lastS5P4Update = millis();
}

void CareerQuestTriggers::updateS5P4LedVisualization() {
    unsigned long now = millis();
    
    if (now - lastS5P4Update < S5P4_UPDATE_INTERVAL) return;
    
    // Handle exit fading
    if (s5p4ExitFading) {
        // Simple fade out - turn off all LEDs
        rgbLed.turn_all_leds_off();
        s5p4Active = false;
        s5p4ExitFading = false;
        lastS5P4Update = now;
        return;
    }
    
    // Get IMU data
    float pitch = SensorDataBuffer::getInstance().getLatestPitch();
    float roll = SensorDataBuffer::getInstance().getLatestRoll();
    
    // Clear all LEDs first
    rgbLed.turn_all_leds_off();
    
    // Define intensity based on tilt magnitude (0-255)
    constexpr float MAX_TILT = 45.0f; // degrees
    constexpr float MIN_TILT = 3.0f;  // deadzone threshold
    
    // Calculate individual LED intensities based on 2D tilt direction
    // LED layout conceptually:
    //     4(top_left)    1(top_right)
    // 3(left_head)  5(mid_left)  0(mid_right)  2(right_head)
    //     6(back_left)   7(back_right)
    
    // Calculate tilt influence for each LED position
    // Front LEDs (headlights): stronger with forward pitch
    float frontInfluence = max(0.0f, -pitch); // negative pitch = forward tilt
    if (frontInfluence > MIN_TILT) {
        uint8_t baseIntensity = (uint8_t)constrain(frontInfluence / MAX_TILT * 255, 0, 255);
        // Headlights get additional intensity from left/right roll
        uint8_t leftHeadIntensity = (uint8_t)constrain(baseIntensity + max(0.0f, -roll) / MAX_TILT * 127, 0, 255);
        uint8_t rightHeadIntensity = (uint8_t)constrain(baseIntensity + max(0.0f, roll) / MAX_TILT * 127, 0, 255);
        strip.setPixelColor(3, strip.Color(0, 0, leftHeadIntensity));  // left_headlight
        strip.setPixelColor(2, strip.Color(0, 0, rightHeadIntensity)); // right_headlight
    }
    
    // Back LEDs: stronger with backward pitch
    float backInfluence = max(0.0f, pitch); // positive pitch = backward tilt
    if (backInfluence > MIN_TILT) {
        uint8_t baseIntensity = (uint8_t)constrain(backInfluence / MAX_TILT * 255, 0, 255);
        uint8_t leftBackIntensity = (uint8_t)constrain(baseIntensity + max(0.0f, -roll) / MAX_TILT * 127, 0, 255);
        uint8_t rightBackIntensity = (uint8_t)constrain(baseIntensity + max(0.0f, roll) / MAX_TILT * 127, 0, 255);
        strip.setPixelColor(6, strip.Color(0, 0, leftBackIntensity));  // back_left
        strip.setPixelColor(7, strip.Color(0, 0, rightBackIntensity)); // back_right
    }
    
    // Top LEDs: influenced by both pitch (front/back) and roll (left/right)
    float topLeftInfluence = max(0.0f, -roll) + max(0.0f, -pitch) * 0.5f; // left roll + forward pitch
    float topRightInfluence = max(0.0f, roll) + max(0.0f, -pitch) * 0.5f;  // right roll + forward pitch
    
    if (topLeftInfluence > MIN_TILT) {
        uint8_t intensity = (uint8_t)constrain(topLeftInfluence / MAX_TILT * 255, 0, 255);
        strip.setPixelColor(4, strip.Color(0, 0, intensity)); // top_left
    }
    if (topRightInfluence > MIN_TILT) {
        uint8_t intensity = (uint8_t)constrain(topRightInfluence / MAX_TILT * 255, 0, 255);
        strip.setPixelColor(1, strip.Color(0, 0, intensity)); // top_right
    }
    
    // Middle LEDs: primarily controlled by roll, with slight pitch influence
    float middleLeftInfluence = max(0.0f, -roll) + abs(pitch) * 0.3f;  // left roll + any pitch
    float middleRightInfluence = max(0.0f, roll) + abs(pitch) * 0.3f;   // right roll + any pitch
    
    if (middleLeftInfluence > MIN_TILT) {
        uint8_t intensity = (uint8_t)constrain(middleLeftInfluence / MAX_TILT * 255, 0, 255);
        strip.setPixelColor(5, strip.Color(0, 0, intensity)); // middle_left
    }
    if (middleRightInfluence > MIN_TILT) {
        uint8_t intensity = (uint8_t)constrain(middleRightInfluence / MAX_TILT * 255, 0, 255);
        strip.setPixelColor(0, strip.Color(0, 0, intensity)); // middle_right
    }
    
    strip.show();
    lastS5P4Update = now;
}

void CareerQuestTriggers::stopAllCareerQuestTriggers() {
    // Stop all active career quest triggers (these will start fade sequences)
    stopS2P1Sequence();
    stopS2P4LightShow();
    stopS3P3DisplayDemo();
    stopS7P4ButtonDemo();
    stopS5P4LedVisualization();
    
    // Stop all LED animations (but don't force LEDs off - let career quest fades complete)
    ledAnimations.stopAnimation();
    rgbLed.turn_all_leds_off();

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
    DanceManager::getInstance().stopDance();
    GameManager::getInstance().stopCurrentGame();
    DisplayScreen::getInstance().showStartScreen();
    SensorDataBuffer::getInstance().stopPollingAllSensors();
}
