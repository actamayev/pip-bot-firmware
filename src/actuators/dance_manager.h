#pragma once

#include <Arduino.h>

#include "actuators/led/led_animations.h"
#include "actuators/motor_driver.h"
#include "networking/serial_queue_manager.h"
#include "utils/singleton.h"

class DanceManager : public Singleton<DanceManager> {
    friend class Singleton<DanceManager>;

  public:
    void start_dance();
    void stop_dance(bool shouldTurnLedsOff);
    void update();
    bool is_dancing() const {
        return _isCurrentlyDancing;
    }

  private:
    DanceManager() = default;

    bool _isCurrentlyDancing = false;
    int _currentStep = 0;
    uint64_t _stepStartTime = 0;
    uint64_t _nextStepTime = 0;

    // Safe dance parameters - much lower than MAX_MOTOR_PWM (4095)
    static constexpr int16_t DANCE_SPEED_GENTLE = 700;   // Very gentle for safety
    static constexpr int16_t DANCE_SPEED_MODERATE = 900; // Still safe but more expressive

    // Dance step structure
    struct DanceStep {
        int16_t leftSpeed;
        int16_t rightSpeed;
        int duration; // milliseconds
        led_types::AnimationType ledAnimation;
        int ledSpeed;
    };

    static constexpr const DanceStep DANCE_SEQUENCE[] = {
        // Opening flourish - slow rainbow wiggles
        {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, led_types::AnimationType::RAINBOW, 10}, // Slow right turn
        {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 400, led_types::AnimationType::RAINBOW, 10}, // Slow left turn
        {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, led_types::AnimationType::RAINBOW, 10}, // Slow right turn

        // Playful spins - cute strobe flashes
        {DANCE_SPEED_MODERATE, -DANCE_SPEED_MODERATE, 600, led_types::AnimationType::STROBING, 6}, // Spin right
        {-DANCE_SPEED_MODERATE, DANCE_SPEED_MODERATE, 600, led_types::AnimationType::STROBING, 6}, // Spin left

        // Gentle sway - small forward/back with breathing
        {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 250, led_types::AnimationType::BREATHING, 8}, // Right turn
        {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 250, led_types::AnimationType::BREATHING, 8}, // Left turn

        // Extra turning section for "cuteness"
        {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 500, led_types::AnimationType::RAINBOW, 12}, // Circle right
        {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 500, led_types::AnimationType::RAINBOW, 12}, // Circle left

        // Final bow - fade out with pause
        {0, 0, 1000, led_types::AnimationType::BREATHING, 0}, // Hold + fade lights
        {0, 0, 500, led_types::AnimationType::NONE, 0}        // End stop
    };

    static constexpr size_t DANCE_SEQUENCE_LENGTH = sizeof(DANCE_SEQUENCE) / sizeof(DANCE_SEQUENCE[0]);
};
