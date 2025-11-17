#include "timeout_manager.h"

#include "actuators/buttons.h"
#include "actuators/led/rgb_led.h"
#include "custom_interpreter/bytecode_vm.h"
#include "networking/serial_queue_manager.h"

void TimeoutManager::resetActivity() {
    lastActivityTime = millis();

    // If we were in confirmation state, cancel it
    if (!inConfirmationState) return;
    cancelConfirmation();
}

void TimeoutManager::update() {
    unsigned long current_time = millis();

    if (!inConfirmationState) {
        // Check for initial inactivity timeout
        if (current_time - lastActivityTime >= INACTIVITY_TIMEOUT) {
            enterConfirmationState();
        }
    } else {
        // Check for confirmation timeout
        if (current_time - confirmationStartTime >= CONFIRMATION_TIMEOUT) {
            Buttons::get_instance().enter_deep_sleep();
        }
    }
}

void TimeoutManager::enterConfirmationState() {
    // Stop bytecode and prepare for sleep (same as long press logic)
    BytecodeVM::get_instance().stopProgram();
    SensorDataBuffer::get_instance().stopPollingAllSensors();
    rgbLed.set_led_yellow();

    inConfirmationState = true;
    confirmationStartTime = millis();
}

void TimeoutManager::cancelConfirmation() {
    if (!inConfirmationState) return;

    inConfirmationState = false;
            rgbLed.turn_all_leds_off();

    // Reset activity timer
    lastActivityTime = millis();
}
