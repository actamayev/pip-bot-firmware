#pragma once

#include <Arduino.h>
#include "utils/singleton.h"
#include "actuators/motor_driver.h"
#include "actuators/led/led_animations.h"
#include "networking/serial_queue_manager.h"

class DanceManager : public Singleton<DanceManager> {
    friend class Singleton<DanceManager>;
    
    public:
        void startDance();
        void stopDance();
        void update();
        bool isDancing() const { return isCurrentlyDancing; }
        
    private:
        DanceManager() = default;
        
        bool isCurrentlyDancing = false;
        int currentStep = 0;
        unsigned long stepStartTime = 0;
        unsigned long nextStepTime = 0;
        
        // Safe dance parameters - much lower than MAX_MOTOR_SPEED (255)
        static constexpr int16_t DANCE_SPEED_GENTLE = 35;  // Very gentle for safety
        static constexpr int16_t DANCE_SPEED_MODERATE = 45; // Still safe but more expressive
        
        // Dance step structure
        struct DanceStep {
            int16_t leftSpeed;
            int16_t rightSpeed;
            int duration; // milliseconds
            LedTypes::AnimationType ledAnimation;
            int ledSpeed;
        };

        static constexpr const DanceStep danceSequence[] = {
            // Opening flourish - slow rainbow wiggles
            {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 10},  // Slow right turn
            {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 10}, // Slow left turn
            {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 10}, // Slow right turn
            
            // Playful spins - cute strobe flashes
            {DANCE_SPEED_MODERATE, -DANCE_SPEED_MODERATE, 600, LedTypes::STROBING, 6}, // Spin right
            {-DANCE_SPEED_MODERATE, DANCE_SPEED_MODERATE, 600, LedTypes::STROBING, 6}, // Spin left
            
            // Gentle sway - small forward/back with breathing
            {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 250, LedTypes::BREATHING, 8},   // Right turn
            {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 250, LedTypes::BREATHING, 8}, // Left turn
            
            // Extra turning section for "cuteness"
            {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 500, LedTypes::RAINBOW, 12},  // Circle right
            {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 500, LedTypes::RAINBOW, 12},  // Circle left
            
            // Final bow - fade out with pause
            {0, 0, 1000, LedTypes::BREATHING, 0},  // Hold + fade lights
            {0, 0, 500, LedTypes::NONE, 0}        // End stop
        };

        static constexpr size_t DANCE_SEQUENCE_LENGTH = sizeof(danceSequence) / sizeof(danceSequence[0]);
};
