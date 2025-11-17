#include "career_quest_triggers.h"

#include <math.h>

#include <algorithm>
#include <cmath>

#include "sensors/sensor_data_buffer.h"

extern Adafruit_NeoPixel strip;
CareerQuestTriggers career_quest_triggers(strip);

// Define static constexpr arrays
constexpr uint8_t CareerQuestTriggers::S2P1_LED_SEQUENCE[8];
constexpr uint8_t CareerQuestTriggers::S2P1_COLOR_SEQUENCE[8][3];
constexpr uint32_t CareerQuestTriggers::S5P4_UPDATE_INTERVAL;
constexpr uint32_t CareerQuestTriggers::S2P4_DURATION;
constexpr uint32_t CareerQuestTriggers::S2P4_SNAKE_DURATION;
constexpr uint32_t CareerQuestTriggers::S2P4_FLASH_DURATION;
constexpr uint32_t CareerQuestTriggers::S2P4_BREATHING_DURATION;
constexpr uint32_t CareerQuestTriggers::S2P4_SNAKE_INTERVAL;
constexpr uint32_t CareerQuestTriggers::S2P4_FLASH_INTERVAL;

CareerQuestTriggers::CareerQuestTriggers(Adafruit_NeoPixel& strip) : _strip(strip) {}

void CareerQuestTriggers::start_s2_p1_sequence() {
    // Turn off all LEDs first
    rgb_led.turn_all_leds_off();

    // Initialize sequence state
    _s2p1Active = true;
    _currentLedIndex = 0;
    _currentColorIndex = 0;
    _lastS2P1Update = millis();
    _isFadingOut = false;
    _currentBrightness = 0;
    _targetBrightness = 255;
}

void CareerQuestTriggers::stop_s2_p1_sequence() {
    if (!career_quest_triggers._s2p1Active) {
        return;
    }
    // Start exit fade instead of immediately turning off
    career_quest_triggers._isExitFading = true;
    career_quest_triggers._isFadingOut = true;
}

void CareerQuestTriggers::start_s2_p4_light_show() {
    // Turn off all LEDs first
    rgb_led.turn_all_leds_off();
    // Stop any existing LED animations
    led_animations.stop_animation();

    // Initialize light show state
    _s2p4Active = true;
    _s2p4StartTime = millis();
    _lastS2P4Update = millis();
    _s2p4Step = 0;
    _s2p4Phase = 0; // Start with snake
    _s2p4SnakePosition = 0;
    _s2p4FlashColorIndex = 0;
    _s2p4SnakeDirection = true;
    _s2p4PhaseStartTime = millis();
}

void CareerQuestTriggers::stop_s2_p4_light_show() {
    if (!career_quest_triggers._s2p4Active) {
        return;
    }
    // Stop any breathing animation immediately
    led_animations.stop_animation();
    // Start exit fade instead of immediately turning off
    career_quest_triggers._s2p4ExitFading = true;
    career_quest_triggers._s2p4CurrentBrightness = 255;
}

void CareerQuestTriggers::update() {
    if (_s2p1Active) {
        update_s2_p1_sequence();
    }
    if (_s2p4Active) {
        update_s2_p4_light_show();
    }
    if (_s7p4Active) {
        update_s7_p4_button_demo();
    }
    if (_s5p4Active) {
        update_s5_p4_led_visualization();
    }
}

void CareerQuestTriggers::update_s2_p1_sequence() {
    uint32_t now = millis();

    if (now - career_quest_triggers._lastS2P1Update < S2P1_UPDATE_INTERVAL) {
        return;
    }
    uint8_t current_led = S2P1_LED_SEQUENCE[career_quest_triggers._currentLedIndex];
    uint8_t red = S2P1_COLOR_SEQUENCE[career_quest_triggers._currentColorIndex][0];
    uint8_t green = S2P1_COLOR_SEQUENCE[career_quest_triggers._currentColorIndex][1];
    uint8_t blue = S2P1_COLOR_SEQUENCE[career_quest_triggers._currentColorIndex][2];

    if (career_quest_triggers._isFadingOut) {
        // Fade out current LED
        if (career_quest_triggers._currentBrightness > 0) {
            career_quest_triggers._currentBrightness = max(0, career_quest_triggers._currentBrightness - S2P1_BRIGHTNESS_STEP); // Fade out step

            // Apply brightness to current LED
            uint8_t faded_red = (red * career_quest_triggers._currentBrightness) / 255;
            uint8_t faded_green = (green * career_quest_triggers._currentBrightness) / 255;
            uint8_t faded_blue = (blue * career_quest_triggers._currentBrightness) / 255;

            career_quest_triggers._strip.setPixelColor(current_led, Adafruit_NeoPixel::Color(faded_red, faded_green, faded_blue));
            career_quest_triggers._strip.show();
        } else {
            if (career_quest_triggers._isExitFading) {
                // Exit fade complete, stop the sequence
                career_quest_triggers._s2p1Active = false;
                career_quest_triggers._isExitFading = false;
                career_quest_triggers._isFadingOut = false;
            } else {
                // Normal fade out complete, move to next LED and start fade in
                career_quest_triggers._isFadingOut = false;
                career_quest_triggers._currentLedIndex = (career_quest_triggers._currentLedIndex + 1) % 8;
                career_quest_triggers._currentColorIndex = (career_quest_triggers._currentColorIndex + 1) % 8;
                career_quest_triggers._currentBrightness = 0;
                career_quest_triggers._targetBrightness = 255;
            }
        }
    } else {
        // Fade in new LED
        if (career_quest_triggers._currentBrightness < career_quest_triggers._targetBrightness) {
            career_quest_triggers._currentBrightness = min(255, career_quest_triggers._currentBrightness + S2P1_BRIGHTNESS_STEP); // Fade in step

            // Apply brightness to new LED with new color
            red = S2P1_COLOR_SEQUENCE[career_quest_triggers._currentColorIndex][0];
            green = S2P1_COLOR_SEQUENCE[career_quest_triggers._currentColorIndex][1];
            blue = S2P1_COLOR_SEQUENCE[career_quest_triggers._currentColorIndex][2];

            uint8_t faded_red = (red * career_quest_triggers._currentBrightness) / 255;
            uint8_t faded_green = (green * career_quest_triggers._currentBrightness) / 255;
            uint8_t faded_blue = (blue * career_quest_triggers._currentBrightness) / 255;

            current_led = S2P1_LED_SEQUENCE[career_quest_triggers._currentLedIndex];
            career_quest_triggers._strip.setPixelColor(current_led, Adafruit_NeoPixel::Color(faded_red, faded_green, faded_blue));
            career_quest_triggers._strip.show();
        } else {
            // Fade in complete, start fade out
            career_quest_triggers._isFadingOut = true;
        }
    }

    career_quest_triggers._lastS2P1Update = now;
}

void CareerQuestTriggers::update_s2_p4_light_show() {
    uint32_t now = millis();

    // Handle exit fading
    if (career_quest_triggers._s2p4ExitFading) {
        // Stop any breathing animation immediately and turn off all LEDs
        led_animations.stop_animation();
        rgb_led.turn_all_leds_off();
        career_quest_triggers._s2p4Active = false;
        career_quest_triggers._s2p4ExitFading = false;
        return;
    }

    // Check total duration
    if (now - career_quest_triggers._s2p4StartTime >= S2P4_DURATION) {
        stop_s2_p4_light_show();
        return;
    }

    uint32_t phase_elapsed = now - career_quest_triggers._s2p4PhaseStartTime;

    // Phase transitions
    if (career_quest_triggers._s2p4Phase == 0 && phase_elapsed >= S2P4_SNAKE_DURATION) {
        // Transition to flash phase
        career_quest_triggers._s2p4Phase = 1;
        career_quest_triggers._s2p4PhaseStartTime = now;
        career_quest_triggers._s2p4FlashColorIndex = 0;
        rgb_led.turn_all_leds_off();
        return;
    } else if (career_quest_triggers._s2p4Phase == 1 && phase_elapsed >= S2P4_FLASH_DURATION) {
        // Transition to breathing phase
        career_quest_triggers._s2p4Phase = 2;
        career_quest_triggers._s2p4PhaseStartTime = now;
        rgb_led.turn_all_leds_off();
        // Start breathing animation with first S2P1 color (red)
        uint8_t r = S2P1_COLOR_SEQUENCE[0][0];
        uint8_t g = S2P1_COLOR_SEQUENCE[0][1];
        uint8_t b = S2P1_COLOR_SEQUENCE[0][2];
        rgb_led.set_default_colors(r, g, b);
        led_animations.start_breathing(2000, 0.0f); // 2s breathing cycle
        return;
    }

    // Phase 0: Snake animation
    if (career_quest_triggers._s2p4Phase == 0) {
        if (now - career_quest_triggers._lastS2P4Update >= S2P4_SNAKE_INTERVAL) {
            rgb_led.turn_all_leds_off();

            // Use S2P1 color sequence for snake
            uint8_t color_index = career_quest_triggers._s2p4SnakePosition % 8;
            uint8_t r = S2P1_COLOR_SEQUENCE[color_index][0];
            uint8_t g = S2P1_COLOR_SEQUENCE[color_index][1];
            uint8_t b = S2P1_COLOR_SEQUENCE[color_index][2];

            // Light up current LED in snake sequence
            uint8_t current_led = S2P1_LED_SEQUENCE[career_quest_triggers._s2p4SnakePosition];
            career_quest_triggers._strip.setPixelColor(current_led, Adafruit_NeoPixel::Color(r, g, b));
            career_quest_triggers._strip.show();

            // Advance snake position
            if (career_quest_triggers._s2p4SnakeDirection) {
                career_quest_triggers._s2p4SnakePosition++;
                if (career_quest_triggers._s2p4SnakePosition >= 8) {
                    career_quest_triggers._s2p4SnakePosition = 6; // Start going back
                    career_quest_triggers._s2p4SnakeDirection = false;
                }
            } else {
                if (career_quest_triggers._s2p4SnakePosition > 0) {
                    career_quest_triggers._s2p4SnakePosition--;
                } else {
                    career_quest_triggers._s2p4SnakeDirection = true;
                    career_quest_triggers._s2p4SnakePosition = 1; // Start going forward again
                }
            }

            career_quest_triggers._lastS2P4Update = now;
        }
    }
    // Phase 1: Flash all LEDs with ROY G BIV colors
    else if (career_quest_triggers._s2p4Phase == 1) {
        if (now - career_quest_triggers._lastS2P4Update >= S2P4_FLASH_INTERVAL) {
            // ROY G BIV color sequence
            uint8_t roygbiv_colors[][3] = {
                {255, 0, 0},   // Red
                {255, 127, 0}, // Orange
                {255, 255, 0}, // Yellow
                {0, 255, 0},   // Green
                {0, 0, 255},   // Blue
                {75, 0, 130},  // Indigo
                {148, 0, 211}  // Violet
            };

            uint8_t color_index = career_quest_triggers._s2p4FlashColorIndex % 7;
            uint8_t red = roygbiv_colors[color_index][0];
            uint8_t green = roygbiv_colors[color_index][1];
            uint8_t blue = roygbiv_colors[color_index][2];

            // Flash all LEDs with current color
            for (unsigned char ledIndex : S2P1_LED_SEQUENCE) {
                career_quest_triggers._strip.setPixelColor(ledIndex, Adafruit_NeoPixel::Color(red, green, blue));
            }
            career_quest_triggers._strip.show();

            career_quest_triggers._s2p4FlashColorIndex++;
            career_quest_triggers._lastS2P4Update = now;
        }
    }
    // Phase 2: Breathing animation (handled by LED animations system)
    // No update needed here - led_animations.update() handles it
}

void CareerQuestTriggers::start_s3_p3_display_demo() {
    _s3p3Active = true;
    _s3p3StartTime = millis();
    _lastS3P3Update = millis();
    _s3p3AnimationStep = 0;
    DisplayScreen::get_instance()._displayOff = false;
}

void CareerQuestTriggers::stop_s3_p3_display_demo() {
    _s3p3Active = false;
}

void CareerQuestTriggers::render_s3_p3_animation() {
    if (!career_quest_triggers._s3p3Active) {
        return;
    }

    uint32_t now = millis();
    if (now - career_quest_triggers._lastS3P3Update >= S3P3_UPDATE_INTERVAL) {
        DisplayScreen& display_screen = DisplayScreen::get_instance();
        Adafruit_SSD1306& display = display_screen._display;

        display.clearDisplay();

        // Calculate elapsed time and loop:
        // Phase1: 0-5s  -> HAFTR scroll
        // Phase2: 5-10s -> 3D cube
        // Phase3: 10-15s -> speed gauge (5s: 2.5s up, 2.5s down)
        uint32_t elapsed_time = now - career_quest_triggers._s3p3StartTime;
        const uint32_t PH1 = 5000UL;
        const uint32_t PH2 = 5000UL;
        const uint32_t PH3 = 5000UL;
        const uint32_t TOTAL = PH1 + PH2 + PH3; // 15000 ms
        uint32_t cycle_time = elapsed_time % TOTAL;

        if (cycle_time < PH1) {
            // Phase 1: HAFTR scrolling (0-5 seconds)
            float progress = static_cast<float>(cycle_time) / static_cast<float>(PH1); // 0.0 .. 1.0

            display.setTextSize(3.5);
            display.setTextColor(SSD1306_WHITE);

            // Compute text width so scroll range adapts to word size
            int16_t xb = 0;
            int16_t yb = 0;
            uint16_t tw = 0;
            uint16_t th = 0;
            const char* word = "HAFTR";
            display.getTextBounds(word, 0, 0, &xb, &yb, &tw, &th);

            int start_x = -static_cast<int>(tw); // start fully off-left
            int end_x = SCREEN_WIDTH + 30;       // end past the right edge
            int text_x = start_x + static_cast<int>(progress * (end_x - start_x));
            int text_y = 24;

            display.setTextWrap(false); // prevent automatic wrapping
            display.setCursor(text_x, text_y);
            display.print(word);
            display.setTextWrap(true); // restore wrap
        } else if (cycle_time < (PH1 + PH2)) {
            // Phase 2: 3D rotating cube (5-10 seconds) — half the previous rotation speed
            uint32_t t = cycle_time - PH1; // 0..PH2-1

            // half-speed rotations (previously 0.003f / 0.004f / 0.0025f)
            float ax = static_cast<float>(t) * 0.0015f;
            float ay = static_cast<float>(t) * 0.0020f;
            float az = static_cast<float>(t) * 0.00125f;

            const int CENTER_X = 64;
            const int CENTER_Y = 35;
            const float SIZE_PIX = 12.0f; // cube half-size in model units (tweak to scale)
            const float FX = 95.0f;       // focal length
            const float ZCAM = 60.0f;     // camera distance

            // Cube verts and edges
            struct Vec3 {
                float x, y, z;
            };
            static const Vec3 CUBE[8] = {{-1, -1, -1}, {+1, -1, -1}, {+1, +1, -1}, {-1, +1, -1},
                                         {-1, -1, +1}, {+1, -1, +1}, {+1, +1, +1}, {-1, +1, +1}};
            static const uint8_t EDGES[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

            // Precompute sin/cos
            float sx = sin(ax);
            float cxr = cos(ax);
            float sy = sin(ay);
            float cyr = cos(ay);
            float sz = sin(az);
            float czr = cos(az);

            float px[8];
            float py[8];
            for (uint8_t i = 0; i < 8; i++) {
                float x = CUBE[i].x * SIZE_PIX;
                float y = CUBE[i].y * SIZE_PIX;
                float z = CUBE[i].z * SIZE_PIX;

                // Rotate X
                float y1 = (y * cxr) - (z * sx);
                float z1 = (y * sx) + (z * cxr);
                float x1 = x;

                // Rotate Y
                float x2 = (x1 * cyr) + (z1 * sy);
                float z2 = (-x1 * sy) + (z1 * cyr);
                float y2 = y1;

                // Rotate Z
                float x3 = (x2 * czr) - (y2 * sz);
                float y3 = (x2 * sz) + (y2 * czr);
                float z3 = z2;

                // Project
                float denom = (z3 + ZCAM);
                if (denom <= 1.0f) {
                    denom = 1.0f;
                }
                float s = FX / denom;

                px[i] = CENTER_X + (x3 * s);
                py[i] = CENTER_Y + (y3 * s);
            }

            // Draw edges
            for (const auto* e : EDGES) {
                uint8_t i0 = e[0];
                uint8_t i1 = e[1];
                display.drawLine(static_cast<int16_t>(px[i0]), static_cast<int16_t>(py[i0]), static_cast<int16_t>(px[i1]),
                                 static_cast<int16_t>(py[i1]), SSD1306_WHITE);
            }
        } else {
            // Phase 3: Speed gauge (10-15s) -> 5 seconds total: 2.5s up, 2.5s down
            uint32_t t = cycle_time - (PH1 + PH2); // 0 .. PH3-1
            const uint32_t HALF = PH3 / 2;         // 2500 ms
            float progress = NAN;
            if (t < HALF) {
                progress = static_cast<float>(t) / static_cast<float>(HALF); // 0..1 (rev up)
            } else {
                progress = 1.0f - (static_cast<float>(t - HALF) / static_cast<float>(HALF)); // 1..0 (rev down)
            }

            // Gauge geometry
            const int CX = 64;
            const int CY = 48;
            const int RADIUS = 22;
            const float START_ANGLE = -3.0f * PI / 4.0f; // -135 degrees
            const float END_ANGLE = 3.0f * PI / 4.0f;    // +135 degrees
            const float SWEEP = END_ANGLE - START_ANGLE;

            // Draw outer arc (approximate by plotting pixels)
            for (float a = START_ANGLE; a <= END_ANGLE; a += 0.04f) {
                int x = CX + (int)(cos(a) * RADIUS);
                int y = CY + (int)(sin(a) * RADIUS);
                display.drawPixel(x, y, SSD1306_WHITE);
            }

            // Draw ticks (major ticks every 15 degrees)
            const int NUM_TICKS = 11;
            for (int i = 0; i < NUM_TICKS; ++i) {
                float ta = START_ANGLE + (SWEEP * static_cast<float>(i) / (NUM_TICKS - 1));
                int x1 = CX + (int)(cos(ta) * (RADIUS - 1));
                int y1 = CY + (int)(sin(ta) * (RADIUS - 1));
                int x2 = CX + (int)(cos(ta) * (RADIUS - 5));
                int y2 = CY + (int)(sin(ta) * (RADIUS - 5));
                display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
            }

            // Draw filled progress arc (draw small radial points from start to current angle)
            float cur_angle = START_ANGLE + (progress * SWEEP);
            for (float a = START_ANGLE; a <= cur_angle; a += 0.03f) {
                int x = CX + (int)(cos(a) * (RADIUS - 2));
                int y = CY + (int)(sin(a) * (RADIUS - 2));
                display.drawPixel(x, y, SSD1306_WHITE);
            }

            // Draw center pivot
            display.fillCircle(CX, CY, 2, SSD1306_WHITE);

            // Draw needle
            int nx = CX + (int)(cos(cur_angle) * (RADIUS - 6));
            int ny = CY + (int)(sin(cur_angle) * (RADIUS - 6));
            display.drawLine(CX, CY, nx, ny, SSD1306_WHITE);
            // small extra line to make needle thicker
            display.drawLine(CX + 1, CY, nx + 1, ny, SSD1306_WHITE);

            // Draw numeric speed (0-100)
            int speed_value = static_cast<int>((progress * 100.0f) + 0.5f);
            char buf[6];
            snprintf(buf, sizeof(buf), "%3d", speed_value);
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            int16_t tbx = 0;
            int16_t tby = 0;
            uint16_t tbw = 0;
            uint16_t tbh = 0;
            display.getTextBounds(buf, 0, 0, &tbx, &tby, &tbw, &tbh);
            // position the number above the gauge center
            display.setCursor(CX - (tbw / 2), CY - RADIUS - 12);
            display.print(buf);

            // Label
            display.setTextSize(1);
            display.setCursor(CX - 16, CY + 8);
            display.print("SPEED");
        }

        career_quest_triggers._s3p3AnimationStep++;
        career_quest_triggers._lastS3P3Update = now;
    }
}

void CareerQuestTriggers::start_s7_p4_button_demo() {
    _s7p4Active = true;
    _s7p4ExitFading = false;
    _s7p4CurrentBrightness = 255;

    // Set up button handlers for random color changes and tones
    Buttons::get_instance().set_left_button_click_handler([](Button2& btn) {
        // Generate random color
        uint8_t red = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t green = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t blue = random(0, MAX_LED_BRIGHTNESS + 1);

        // Fade to new color using breathing animation
        rgb_led.set_default_colors(red, green, blue);
        led_animations.start_breathing(1000, 0.0f); // 1s fade from dark
    });

    Buttons::get_instance().set_right_button_click_handler([](Button2& btn) {
        // Generate random color
        uint8_t red = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t green = random(0, MAX_LED_BRIGHTNESS + 1);
        uint8_t blue = random(0, MAX_LED_BRIGHTNESS + 1);

        // Fade to new color using breathing animation
        rgb_led.set_default_colors(red, green, blue);
        led_animations.start_breathing(1000, 0.0f); // 1s fade from dark
    });
}

void CareerQuestTriggers::stop_s7_p4_button_demo() {
    if (!career_quest_triggers._s7p4Active) {
        return;
    }

    // Clear button handlers (set to nullptr)
    Buttons::get_instance().set_left_button_click_handler(nullptr);
    Buttons::get_instance().set_right_button_click_handler(nullptr);

    // Start exit fade instead of immediately turning off
    career_quest_triggers._s7p4ExitFading = true;
    career_quest_triggers._s7p4CurrentBrightness = 255;
    career_quest_triggers._lastS7P4Update = millis();
}

void CareerQuestTriggers::update_s7_p4_button_demo() {
    if (!career_quest_triggers._s7p4ExitFading) {
        return;
    }

    uint32_t now = millis();
    if (now - career_quest_triggers._lastS7P4Update < S2P1_UPDATE_INTERVAL) {
        return;
    }

    // Fade out all main board LEDs
    if (career_quest_triggers._s7p4CurrentBrightness > 0) {
        career_quest_triggers._s7p4CurrentBrightness = max(0, career_quest_triggers._s7p4CurrentBrightness - 5);

        // Apply brightness to all main board LEDs
        uint8_t faded_red = (rgb_led.get_current_red() * career_quest_triggers._s7p4CurrentBrightness) / 255;
        uint8_t faded_green = (rgb_led.get_current_green() * career_quest_triggers._s7p4CurrentBrightness) / 255;
        uint8_t faded_blue = (rgb_led.get_current_blue() * career_quest_triggers._s7p4CurrentBrightness) / 255;

        rgb_led.set_main_board_leds_to_color(faded_red, faded_green, faded_blue);
    } else {
        // Fade complete, stop the demo
        career_quest_triggers._s7p4Active = false;
        career_quest_triggers._s7p4ExitFading = false;
        rgb_led.turn_all_leds_off();
    }

    career_quest_triggers._lastS7P4Update = now;
}

void CareerQuestTriggers::start_s5_p4_led_visualization() {
    rgb_led.turn_all_leds_off();
    _s5p4Active = true;
    _s5p4ExitFading = false;
    _lastS5P4Update = millis();
}

void CareerQuestTriggers::stop_s5_p4_led_visualization() {
    if (!career_quest_triggers._s5p4Active) {
        return;
    }
    career_quest_triggers._s5p4ExitFading = true;
    career_quest_triggers._lastS5P4Update = millis();
}

void CareerQuestTriggers::update_s5_p4_led_visualization() {
    uint32_t now = millis();

    if (now - career_quest_triggers._lastS5P4Update < S5P4_UPDATE_INTERVAL) {
        return;
    }

    // Handle exit fading
    if (career_quest_triggers._s5p4ExitFading) {
        // Simple fade out - turn off all LEDs
        rgb_led.turn_all_leds_off();
        career_quest_triggers._s5p4Active = false;
        career_quest_triggers._s5p4ExitFading = false;
        career_quest_triggers._lastS5P4Update = now;
        return;
    }

    // Get IMU data
    float pitch = SensorDataBuffer::get_instance().get_latest_pitch();
    float roll = SensorDataBuffer::get_instance().get_latest_roll();

    // Clear all LEDs first
    rgb_led.turn_all_leds_off();

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
    float front_influence = max(0.0f, -pitch); // negative pitch = forward tilt
    if (front_influence > MIN_TILT) {
        auto base_intensity = static_cast<uint8_t> constrain(front_influence / MAX_TILT * 255, 0, 255);
        // Headlights get additional intensity from left/right roll
        auto left_head_intensity = (uint8_t)constrain(base_intensity + max(0.0f, -roll) / MAX_TILT * 127, 0, 255);
        auto right_head_intensity = (uint8_t)constrain(base_intensity + max(0.0f, roll) / MAX_TILT * 127, 0, 255);
        career_quest_triggers._strip.setPixelColor(3, Adafruit_NeoPixel::Color(0, 0, left_head_intensity));  // left_headlight
        career_quest_triggers._strip.setPixelColor(2, Adafruit_NeoPixel::Color(0, 0, right_head_intensity)); // right_headlight
    }

    // Back LEDs: stronger with backward pitch
    float back_influence = max(0.0f, pitch); // positive pitch = backward tilt
    if (back_influence > MIN_TILT) {
        auto base_intensity = static_cast<uint8_t> constrain(back_influence / MAX_TILT * 255, 0, 255);
        auto left_back_intensity = (uint8_t)constrain(base_intensity + max(0.0f, -roll) / MAX_TILT * 127, 0, 255);
        auto right_back_intensity = (uint8_t)constrain(base_intensity + max(0.0f, roll) / MAX_TILT * 127, 0, 255);
        career_quest_triggers._strip.setPixelColor(6, Adafruit_NeoPixel::Color(0, 0, left_back_intensity));  // back_left
        career_quest_triggers._strip.setPixelColor(7, Adafruit_NeoPixel::Color(0, 0, right_back_intensity)); // back_right
    }

    // Top LEDs: influenced by both pitch (front/back) and roll (left/right)
    float top_left_influence = max(0.0f, -roll) + max(0.0f, -pitch) * 0.5f; // left roll + forward pitch
    float top_right_influence = max(0.0f, roll) + max(0.0f, -pitch) * 0.5f; // right roll + forward pitch

    if (top_left_influence > MIN_TILT) {
        auto intensity = static_cast<uint8_t> constrain(top_left_influence / MAX_TILT * 255, 0, 255);
        career_quest_triggers._strip.setPixelColor(4, Adafruit_NeoPixel::Color(0, 0, intensity)); // top_left
    }
    if (top_right_influence > MIN_TILT) {
        auto intensity = static_cast<uint8_t> constrain(top_right_influence / MAX_TILT * 255, 0, 255);
        career_quest_triggers._strip.setPixelColor(1, Adafruit_NeoPixel::Color(0, 0, intensity)); // top_right
    }

    // Middle LEDs: primarily controlled by roll, with slight pitch influence
    float middle_left_influence = max(0.0f, -roll) + abs(pitch) * 0.3f; // left roll + any pitch
    float middle_right_influence = max(0.0f, roll) + abs(pitch) * 0.3f; // right roll + any pitch

    if (middle_left_influence > MIN_TILT) {
        auto intensity = static_cast<uint8_t> constrain(middle_left_influence / MAX_TILT * 255, 0, 255);
        career_quest_triggers._strip.setPixelColor(5, Adafruit_NeoPixel::Color(0, 0, intensity)); // middle_left
    }
    if (middle_right_influence > MIN_TILT) {
        auto intensity = static_cast<uint8_t> constrain(middle_right_influence / MAX_TILT * 255, 0, 255);
        career_quest_triggers._strip.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, intensity)); // middle_right
    }

    career_quest_triggers._strip.show();
    career_quest_triggers._lastS5P4Update = now;
}

void CareerQuestTriggers::stop_all_career_quest_triggers(bool should_turn_leds_off) {
    // Stop all active career quest triggers (these will start fade sequences)
    stop_s2_p1_sequence();
    stop_s2_p4_light_show();
    stop_s3_p3_display_demo();
    stop_s7_p4_button_demo();
    stop_s5_p4_led_visualization();

    if (should_turn_leds_off) {
        // Stop all LED animations (but don't force LEDs off - let career quest fades complete)
        led_animations.stop_animation();
        rgb_led.turn_all_leds_off();
    }

    // Stop all audio
    Speaker::get_instance().stop_all_sounds();

    // Disable all sensor data sending
    SendSensorData::get_instance().set_send_sensor_data(false);
    SendSensorData::get_instance().set_send_multizone_data(false);
    SendSensorData::get_instance().set_euler_data_enabled(false);
    SendSensorData::get_instance().set_accel_data_enabled(false);
    SendSensorData::get_instance().set_gyro_data_enabled(false);
    SendSensorData::get_instance().set_magnetometer_data_enabled(false);
    SendSensorData::get_instance().set_multizone_tof_data_enabled(false);
    SendSensorData::get_instance().set_side_tof_data_enabled(false);
    SendSensorData::get_instance().set_color_sensor_data_enabled(false);
    SendSensorData::get_instance().set_encoder_data_enabled(false);

    // Stop motors
    DanceManager::get_instance().stop_dance(false);
    GameManager::get_instance().stop_current_game();
    DisplayScreen::get_instance().show_start_screen();
    SensorDataBuffer::get_instance().stop_polling_all_sensors();
}
