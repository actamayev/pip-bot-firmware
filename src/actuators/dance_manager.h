#pragma once

#include <Arduino.h>
#include "utils/singleton.h"
#include "actuators/motor_driver.h"
#include "actuators/led/led_animations.h"
#include "networking/serial_queue_manager.h"

class DanceManager : public Singleton<DanceManager> {
    friend class Singleton<DanceManager>;
    
    public:
        void startEntertainerDance();
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
        static constexpr int16_t DANCE_SPEED_GENTLE = 60;  // Very gentle for safety
        static constexpr int16_t DANCE_SPEED_MODERATE = 80; // Still safe but more expressive
        
        // Dance step structure
        struct DanceStep {
            int16_t leftSpeed;
            int16_t rightSpeed;
            int duration; // milliseconds
            LedTypes::AnimationType ledAnimation;
            int ledSpeed;
        };
        
        // The Entertainer dance sequence - synchronized with melody
        static constexpr DanceStep entertainerDance[] = {
            // Opening flourish - gentle turns with rainbow
            {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 8},      // Right turn
            {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 8},     // Left turn
            {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 8},     // Right turn
            
            // Main melody section - breathing with forward/back
            {DANCE_SPEED_MODERATE, DANCE_SPEED_MODERATE, 600, LedTypes::BREATHING, 6}, // Forward
            {-DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 300, LedTypes::BREATHING, 6},   // Back gentle
            {DANCE_SPEED_MODERATE, DANCE_SPEED_MODERATE, 600, LedTypes::BREATHING, 6}, // Forward
            
            // Playful spins - strobing lights
            {DANCE_SPEED_MODERATE, -DANCE_SPEED_MODERATE, 800, LedTypes::STROBING, 4}, // Spin right
            {-DANCE_SPEED_MODERATE, DANCE_SPEED_MODERATE, 800, LedTypes::STROBING, 4}, // Spin left
            
            // Final bow - gentle movement with fade
            {DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 400, LedTypes::BREATHING, 10},    // Gentle forward
            {-DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, LedTypes::BREATHING, 10},  // Gentle back
            {0, 0, 1000, LedTypes::NONE, 0}  // Stop and hold with lights off
        };
        
        static constexpr int danceStepCount = sizeof(entertainerDance) / sizeof(entertainerDance[0]);
};
