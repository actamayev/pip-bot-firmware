#include "dance_manager.h"
#include "networking/serial_manager.h"

void DanceManager::startEntertainerDance() {
    // Safety check - only dance if USB is not connected
    if (SerialManager::getInstance().isSerialConnected()) {
        SerialQueueManager::getInstance().queueMessage("Dance blocked - USB connected for safety");
        return;
    }
    
    if (isCurrentlyDancing) {
        SerialQueueManager::getInstance().queueMessage("Dance already in progress");
        return;
    }
    
    SerialQueueManager::getInstance().queueMessage("Starting Entertainer dance sequence");
    isCurrentlyDancing = true;
    currentStep = 0;
    stepStartTime = millis();
    nextStepTime = stepStartTime + entertainerDance[0].duration;
    
    // Start first dance step
    DanceStep firstStep = entertainerDance[0];
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
        if (currentStep >= danceStepCount) {
            stopDance();
            SerialQueueManager::getInstance().queueMessage("Entertainer dance completed");
            return;
        }
        
        // Execute next dance step
        DanceStep step = entertainerDance[currentStep];
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
        
        SerialQueueManager::getInstance().queueMessage("Dance step " + String(currentStep) + "/" + String(danceStepCount));
    }
}
