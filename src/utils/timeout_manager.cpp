#include "timeout_manager.h"

#include "actuators/buttons.h"
#include "actuators/led/rgb_led.h"
#include "custom_interpreter/bytecode_vm.h"
#include "networking/serial_queue_manager.h"

void TimeoutManager::reset_activity() {
    _lastActivityTime = millis();

    // If we were in confirmation state, cancel it
    if (!_inConfirmationState) {
        return;
    }
    cancel_confirmation();
}

void TimeoutManager::update() {
    uint32_t current_time = millis();

    if (!_inConfirmationState) {
        // Check for initial inactivity timeout
        if (current_time - _lastActivityTime >= INACTIVITY_TIMEOUT) {
            enter_confirmation_state();
        }
    } else {
        // Check for confirmation timeout
        if (current_time - _confirmationStartTime >= CONFIRMATION_TIMEOUT) {
            Buttons::get_instance().enter_deep_sleep();
        }
    }
}

void TimeoutManager::enter_confirmation_state() {
    // Stop bytecode and prepare for sleep (same as long press logic)
    BytecodeVM::get_instance().stop_program();
    SensorDataBuffer::get_instance().stop_polling_all_sensors();
    rgb_led.set_led_yellow();

    _inConfirmationState = true;
    _confirmationStartTime = millis();
}

void TimeoutManager::cancel_confirmation() {
    if (!_inConfirmationState) {
        return;
    }

    _inConfirmationState = false;
    rgb_led.turn_all_leds_off();

    // Reset activity timer
    _lastActivityTime = millis();
}
