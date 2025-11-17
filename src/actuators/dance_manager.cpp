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
    const DanceStep FIRST_STEP = DANCE_SEQUENCE[0];
    motor_driver.update_motor_pwm(FIRST_STEP.leftSpeed, FIRST_STEP.rightSpeed);

    // Start LED animation
    if (FIRST_STEP.ledAnimation == led_types::AnimationType::RAINBOW) {
        led_animations.start_rainbow(2000);
    } else if (FIRST_STEP.ledAnimation == led_types::AnimationType::BREATHING) {
        led_animations.start_breathing(2000, 0.5f);
    } else if (FIRST_STEP.ledAnimation == led_types::AnimationType::STROBING) {
        led_animations.start_strobing(500);
    }
}

void DanceManager::stop_dance(bool should_turn_leds_off) {
    if (!_isCurrentlyDancing) {
        return;
    }

    SerialQueueManager::get_instance().queue_message("Stopping dance sequence");
    _isCurrentlyDancing = false;
    _currentStep = 0;

    // Stop motors immediately for safety
    motor_driver.stop_both_motors();

    if (!should_turn_leds_off) {
        return;
    }
    // Turn off LEDs
    led_animations.turn_off();
    rgb_led.turn_all_leds_off();
}

void DanceManager::update() {
    if (!_isCurrentlyDancing) {
        return;
    }

    // Safety check during dance - stop if USB gets connected
    // if (SerialManager::get_instance().isSerialConnected()) {
    //     SerialQueueManager::get_instance().queue_message("USB connected during dance - stopping for safety");
    //     stopDance();
    //     return;
    // }

    const uint32_t CURRENT_TIME = millis();

    // Check if it's time for the next step
    if (CURRENT_TIME < _nextStepTime) {
        return;
    }
    _currentStep++;

    // Check if dance is complete
    if (_currentStep >= DANCE_SEQUENCE_LENGTH) {
        stop_dance(true);
        return;
    }

    // Execute next dance step
    const DanceStep STEP = DANCE_SEQUENCE[_currentStep];
    _stepStartTime = CURRENT_TIME;
    _nextStepTime = CURRENT_TIME + STEP.duration;

    // Update motors with gentle speeds
    motor_driver.update_motor_pwm(STEP.leftSpeed, STEP.rightSpeed);

    // Update LED animation
    if (STEP.ledAnimation == led_types::AnimationType::RAINBOW) {
        led_animations.start_rainbow(2000);
    } else if (STEP.ledAnimation == led_types::AnimationType::BREATHING) {
        led_animations.start_breathing(2000, 0.5f);
    } else if (STEP.ledAnimation == led_types::AnimationType::STROBING) {
        led_animations.start_strobing(500);
    } else if (STEP.ledAnimation == led_types::AnimationType::NONE) {
        led_animations.turn_off();
        rgb_led.turn_all_leds_off();
    }
}
