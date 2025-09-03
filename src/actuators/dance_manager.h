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
        
        // Fun dance sequence - safe movements with light show
        static const DanceStep* getDanceSequence();
        static int getDanceStepCount();
};
