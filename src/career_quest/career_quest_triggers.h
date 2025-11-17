#pragma once
#include "actuators/buttons.h"
#include "actuators/display_screen.h"
#include "actuators/led/led_animations.h"
#include "actuators/led/rgb_led.h"
#include "actuators/speaker.h"
#include "networking/protocol.h"
#include "utils/config.h"
#include "utils/structs.h"

class CareerQuestTriggers {
  public:
    explicit CareerQuestTriggers(Adafruit_NeoPixel& strip);

    void start_s2_p1_sequence();
    static void stop_s2_p1_sequence();
    void start_s2_p4_light_show();
    static void stop_s2_p4_light_show();
    void start_s3_p3_display_demo();
    void stop_s3_p3_display_demo();
    void start_s7_p4_button_demo();
    static void stop_s7_p4_button_demo();
    void start_s5_p4_led_visualization();
    static void stop_s5_p4_led_visualization();
    void stop_all_career_quest_triggers(bool should_turn_leds_off);
    void update();

    bool is_s2_p1_active() const {
        return _s2p1Active;
    }
    bool is_s2_p4_active() const {
        return _s2p4Active;
    }
    bool is_s3_p3_active() const {
        return _s3p3Active;
    }
    bool is_s7_p4_active() const {
        return _s7p4Active;
    }
    bool is_s5_p4_active() const {
        return _s5p4Active;
    }

    static void render_s3_p3_animation();

    // LED sequence order (back_right → middle_right → etc.) including headlights
    static constexpr uint8_t S2P1_LED_SEQUENCE[8] = {7, 0, 1, 2, 3, 4, 5, 6}; // Include headlights (2,3)

    // Color progression (Red → Orange → Yellow → Green → Blue → Purple → Cyan → Magenta)
    static constexpr uint8_t S2P1_COLOR_SEQUENCE[8][3] = {
        {255, 0, 0},   // Red
        {255, 127, 0}, // Orange
        {255, 255, 0}, // Yellow
        {127, 255, 0}, // Chartreuse
        {0, 255, 0},   // Green
        {0, 255, 255}, // Cyan
        {0, 0, 255},   // Blue
        {128, 0, 255}  // Purple
    };

    // Timing constants
    static constexpr uint32_t S2P1_UPDATE_INTERVAL = 10;      // 10ms updates for faster, smoother fading
    static constexpr uint8_t S2P1_BRIGHTNESS_STEP = 10;       // 10ms updates for faster, smoother fading
    static constexpr uint32_t S2P4_DURATION = 15000;          // 15 seconds total
    static constexpr uint32_t S2P4_SNAKE_DURATION = 6000;     // 6 seconds
    static constexpr uint32_t S2P4_FLASH_DURATION = 4000;     // 4 seconds
    static constexpr uint32_t S2P4_BREATHING_DURATION = 5000; // 5 seconds
    static constexpr uint32_t S2P4_SNAKE_INTERVAL = 200;      // 200ms per LED
    static constexpr uint32_t S2P4_FLASH_INTERVAL = 400;      // 400ms per flash
    static constexpr uint32_t S3P3_UPDATE_INTERVAL = 50;      // 50ms updates for smooth scrolling

  private:
    Adafruit_NeoPixel& _strip;

    // S2_P1 sequence state
    bool _s2p1Active = false;
    uint8_t _currentLedIndex = 0;
    uint8_t _currentColorIndex = 0;
    uint32_t _lastS2P1Update = 0;

    // Fade state
    bool _isFadingOut = false;
    bool _isExitFading = false;
    uint8_t _currentBrightness = 0;
    uint8_t _targetBrightness = 255;

    // S2_P4 light show state
    bool _s2p4Active = false;
    uint32_t _s2p4StartTime = 0;
    uint32_t _lastS2P4Update = 0;
    uint8_t _s2p4Step = 0;
    bool _s2p4ExitFading = false;
    uint8_t _s2p4CurrentBrightness = 255;

    // S2P4 3-phase light show
    uint8_t _s2p4Phase = 0; // 0=snake, 1=flash, 2=breathing
    uint8_t _s2p4SnakePosition = 0;
    uint8_t _s2p4FlashColorIndex = 0;
    bool _s2p4SnakeDirection = true; // true=forward, false=reverse
    uint32_t _s2p4PhaseStartTime = 0;

    // S3_P3 display demo state
    bool _s3p3Active = false;
    uint32_t _lastS3P3Update = 0;
    uint32_t _s3p3StartTime = 0;
    uint8_t _s3p3AnimationStep = 0;

    // S7_P4 button demo state
    bool _s7p4Active = false;
    bool _s7p4ExitFading = false;
    uint8_t _s7p4CurrentBrightness = 255;
    uint32_t _lastS7P4Update = 0;

    // S5_P4 IMU LED visualization state
    bool _s5p4Active = false;
    bool _s5p4ExitFading = false;
    uint32_t _lastS5P4Update = 0;
    static constexpr uint32_t S5P4_UPDATE_INTERVAL = 20; // 20ms for responsive visualization

    static void update_s2_p1_sequence();
    static void update_s2_p4_light_show();
    static void update_s7_p4_button_demo();
    static void update_s5_p4_led_visualization();
};

extern CareerQuestTriggers career_quest_triggers;
