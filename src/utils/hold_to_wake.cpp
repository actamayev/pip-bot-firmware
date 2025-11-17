#include "hold_to_wake.h"

#include "actuators/buttons.h"  // Add this include for Buttons
#include "utils/task_manager.h" // Add this include for TaskManager

// Function to check if we should proceed with full startup after deep sleep wake
bool hold_to_wake() {
    // Set hold-to-wake mode immediately to prevent yellow LED flash
    Buttons::get_instance().set_hold_to_wake_mode(true);

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT1) {
        Buttons::get_instance().set_hold_to_wake_mode(false);
        return true;
    }

    // Configure both button pins for reading (changed to INPUT_PULLDOWN)
    pinMode(LEFT_BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(RIGHT_BUTTON_PIN, INPUT_PULLDOWN);

    // Check if any button is still pressed (HIGH due to INPUT_PULLDOWN)
    bool leftButtonPressed = digitalRead(LEFT_BUTTON_PIN) == HIGH;
    bool rightButtonPressed = digitalRead(RIGHT_BUTTON_PIN) == HIGH;

    if (!leftButtonPressed && !rightButtonPressed) {
        Buttons::get_instance().enter_deep_sleep();
        return false;
    }

    // Start timing - at least one button must be held for 1000ms
    const uint32_t HOLD_DURATION_MS = 1000;
    uint32_t startTime = millis();

    while ((millis() - startTime) < HOLD_DURATION_MS) {
        // Re-check button states
        leftButtonPressed = digitalRead(LEFT_BUTTON_PIN) == HIGH;
        rightButtonPressed = digitalRead(RIGHT_BUTTON_PIN) == HIGH;

        // If both buttons are released, go back to sleep
        if (!leftButtonPressed && !rightButtonPressed) {
            uint32_t heldTime = millis() - startTime;
            Buttons::get_instance().enter_deep_sleep();
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // HOLD DURATION REACHED - INITIALIZE DISPLAY IMMEDIATELY for deep sleep wake
    if (!TaskManager::is_display_initialized()) {
        TaskManager::create_display_init_task();
    }

    // Clear hold-to-wake mode (timestamp will be set to ignore immediate long clicks)
    Buttons::get_instance().set_hold_to_wake_mode(false);

    return true; // Proceed with full startup
}
