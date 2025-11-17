#include "dance_manager.h"

#include "networking/serial_manager.h"

// Define the static constexpr array
constexpr const DanceManager::DanceStep DanceManager::DANCE_SEQUENCE[];

void DanceManager::start_dance() {
    // Safety check - only dance if USB is not connected
    // if (SerialManager::get_instance().isSerialConnected()) {
    //     SerialQueueManager::get_instance().queue_message("Dance blocked - USB connected for safety");
    //     return;
    // }

    if (_isCurrentlyDancing) {
        SerialQueueManager::get_instance().queue_message("Dance already in progress");
        return;
    }

    SerialQueueManager::get_instance().queue_message("Starting dance sequence");
    _isCurrentlyDancing = true;
    _currentStep = 0;
    _stepStartTime = millis();

    _nextStepTime = _stepStartTime + DANCE_SEQUENCE[0].duration;

    // Start first dance step
    DanceStep firstStep = DANCE_SEQUENCE[0];
    motorDriver.update_motor_pwm(firstStep.leftSpeed, firstStep.rightSpeed);

    // Start LED animation
    if (firstStep.ledAnimation == led_types::AnimationType::RAINBOW) {
        ledAnimations.start_rainbow(2000);
    } else if (firstStep.ledAnimation == led_types::AnimationType::BREATHING) {
        ledAnimations.start_breathing(2000, 0.5f);
    } else if (firstStep.ledAnimation == led_types::AnimationType::STROBING) {
        ledAnimations.start_strobing(500);
    }
}

void DanceManager::stop_dance(bool shouldTurnLedsOff) {
    if (!_isCurrentlyDancing) return;

    SerialQueueManager::get_instance().queue_message("Stopping dance sequence");
    _isCurrentlyDancing = false;
    _currentStep = 0;

    // Stop motors immediately for safety
    motorDriver.stop_both_motors();

    if (shouldTurnLedsOff) {
        // Turn off LEDs
        ledAnimations.turn_off();
        rgbLed.turn_all_leds_off();
    }
}

void DanceManager::update() {
    if (!_isCurrentlyDancing) return;

    // Safety check during dance - stop if USB gets connected
    // if (SerialManager::get_instance().isSerialConnected()) {
    //     SerialQueueManager::get_instance().queue_message("USB connected during dance - stopping for safety");
    //     stopDance();
    //     return;
    // }

    uint32_t current_time = millis();

    // Check if it's time for the next step
    if (current_time < _nextStepTime) return;
    _currentStep++;

    // Check if dance is complete
    if (_currentStep >= DANCE_SEQUENCE_LENGTH) {
        stop_dance(true);
        return;
    }

    // Execute next dance step
    DanceStep step = DANCE_SEQUENCE[_currentStep];
    _stepStartTime = current_time;
    _nextStepTime = current_time + step.duration;

    // Update motors with gentle speeds
    motorDriver.update_motor_pwm(step.leftSpeed, step.rightSpeed);

    // Update LED animation
    if (step.ledAnimation == led_types::AnimationType::RAINBOW) {
        ledAnimations.start_rainbow(2000);
    } else if (step.ledAnimation == led_types::AnimationType::BREATHING) {
        ledAnimations.start_breathing(2000, 0.5f);
    } else if (step.ledAnimation == led_types::AnimationType::STROBING) {
        ledAnimations.start_strobing(500);
    } else if (step.ledAnimation == led_types::AnimationType::NONE) {
        ledAnimations.turn_off();
        rgbLed.turn_all_leds_off();
    }
}
