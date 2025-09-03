#include "dance_manager.h"
#include "networking/serial_manager.h"

const DanceManager::DanceStep* DanceManager::getDanceSequence() {
    static const DanceStep danceSequence[] = {
        // Opening flourish - slow rainbow wiggles
        {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 10},  // Slow right turn
        {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 10}, // Slow left turn
        {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 400, LedTypes::RAINBOW, 10}, // Slow right turn
        
        // Playful spins - cute strobe flashes
        {DANCE_SPEED_MODERATE, -DANCE_SPEED_MODERATE, 600, LedTypes::STROBING, 6}, // Spin right
        {-DANCE_SPEED_MODERATE, DANCE_SPEED_MODERATE, 600, LedTypes::STROBING, 6}, // Spin left
        
        // Gentle sway - small forward/back with breathing
        {DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 250, LedTypes::BREATHING, 8},   // Tiny forward
        {-DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 250, LedTypes::BREATHING, 8}, // Tiny back
        
        // Extra turning section for "cuteness"
        {DANCE_SPEED_GENTLE, -DANCE_SPEED_GENTLE, 500, LedTypes::RAINBOW, 12},  // Circle right
        {-DANCE_SPEED_GENTLE, DANCE_SPEED_GENTLE, 500, LedTypes::RAINBOW, 12},  // Circle left
        
        // Final bow - fade out with pause
        {0, 0, 1000, LedTypes::BREATHING, 0},  // Hold + fade lights
        {0, 0, 500, LedTypes::NONE, 0}        // End stop
    };
    return danceSequence;
}

int DanceManager::getDanceStepCount() {
    return 11; // Number of steps in the dance sequence
}

void DanceManager::startDance() {
    // Safety check - only dance if USB is not connected
    if (SerialManager::getInstance().isSerialConnected()) {
        SerialQueueManager::getInstance().queueMessage("Dance blocked - USB connected for safety");
        return;
    }
    
    if (isCurrentlyDancing) {
        SerialQueueManager::getInstance().queueMessage("Dance already in progress");
        return;
    }
    
    SerialQueueManager::getInstance().queueMessage("Starting dance sequence");
    isCurrentlyDancing = true;
    currentStep = 0;
    stepStartTime = millis();
    
    const DanceStep* danceSequence = getDanceSequence();
    nextStepTime = stepStartTime + danceSequence[0].duration;
    
    // Start first dance step
    DanceStep firstStep = danceSequence[0];
    motorDriver.updateMotorPwm(firstStep.leftSpeed, firstStep.rightSpeed);
    
    // Start LED animation
    if (firstStep.ledAnimation == LedTypes::RAINBOW) {
        ledAnimations.startRainbow(2000);
    } else if (firstStep.ledAnimation == LedTypes::BREATHING) {
        ledAnimations.startBreathing(2000, 0.5f);
    } else if (firstStep.ledAnimation == LedTypes::STROBING) {
        ledAnimations.startStrobing(500);
    }
}

void DanceManager::stopDance() {
    if (!isCurrentlyDancing) return;
    
    SerialQueueManager::getInstance().queueMessage("Stopping dance sequence");
    isCurrentlyDancing = false;
    currentStep = 0;
    
    // Stop motors immediately for safety
    motorDriver.stop_both_motors();
    
    // Turn off LEDs
    ledAnimations.turnOff();
    rgbLed.turn_all_leds_off();
}

void DanceManager::update() {
    if (!isCurrentlyDancing) return;
    
    // Safety check during dance - stop if USB gets connected
    if (SerialManager::getInstance().isSerialConnected()) {
        SerialQueueManager::getInstance().queueMessage("USB connected during dance - stopping for safety");
        stopDance();
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Check if it's time for the next step
    if (currentTime >= nextStepTime) {
        currentStep++;
        
        // Check if dance is complete
        if (currentStep >= getDanceStepCount()) {
            stopDance();
            SerialQueueManager::getInstance().queueMessage("Dance completed");
            return;
        }
        
        // Execute next dance step
        const DanceStep* danceSequence = getDanceSequence();
        DanceStep step = danceSequence[currentStep];
        stepStartTime = currentTime;
        nextStepTime = currentTime + step.duration;
        
        // Update motors with gentle speeds
        motorDriver.updateMotorPwm(step.leftSpeed, step.rightSpeed);
        
        // Update LED animation
        if (step.ledAnimation == LedTypes::RAINBOW) {
            ledAnimations.startRainbow(2000);
        } else if (step.ledAnimation == LedTypes::BREATHING) {
            ledAnimations.startBreathing(2000, 0.5f);
        } else if (step.ledAnimation == LedTypes::STROBING) {
            ledAnimations.startStrobing(500);
        } else if (step.ledAnimation == LedTypes::NONE) {
            ledAnimations.turnOff();
            rgbLed.turn_all_leds_off();
        }
        
        SerialQueueManager::getInstance().queueMessage("Dance step " + String(currentStep) + "/" + String(getDanceStepCount()));
    }
}
