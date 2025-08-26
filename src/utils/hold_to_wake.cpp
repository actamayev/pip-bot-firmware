#include "hold_to_wake.h"
#include "utils/task_manager.h"  // Add this include for TaskManager
#include "actuators/buttons.h"    // Add this include for Buttons

// Function to check if we should proceed with full startup after deep sleep wake
bool holdToWake() {
    // Set hold-to-wake mode immediately to prevent yellow LED flash
    Buttons::getInstance().setHoldToWakeMode(true);
    
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT1) {
        Buttons::getInstance().setHoldToWakeMode(false);
        return true;
    }
    
    // Configure both button pins for reading (changed to INPUT_PULLDOWN)
    pinMode(BUTTON_PIN_1, INPUT_PULLDOWN);
    pinMode(BUTTON_PIN_2, INPUT_PULLDOWN);
    
    // Check if any button is still pressed (HIGH due to INPUT_PULLDOWN)
    bool button1Pressed = digitalRead(BUTTON_PIN_1) == HIGH;
    bool button2Pressed = digitalRead(BUTTON_PIN_2) == HIGH;
    
    if (!button1Pressed && !button2Pressed) {
        Buttons::getInstance().enterDeepSleep();
        return false;
    }
    
    // Start timing - at least one button must be held for 1000ms
    const uint32_t HOLD_DURATION_MS = 1000;
    uint32_t startTime = millis();
    
    while ((millis() - startTime) < HOLD_DURATION_MS) {
        // Re-check button states
        button1Pressed = digitalRead(BUTTON_PIN_1) == HIGH;
        button2Pressed = digitalRead(BUTTON_PIN_2) == HIGH;
        
        // If both buttons are released, go back to sleep
        if (!button1Pressed && !button2Pressed) {
            uint32_t heldTime = millis() - startTime;
            Buttons::getInstance().enterDeepSleep();
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // HOLD DURATION REACHED - INITIALIZE DISPLAY IMMEDIATELY for deep sleep wake
    // if (!TaskManager::isDisplayInitialized()) {
    //     TaskManager::createDisplayInitTask();
    // }
    
    // Clear hold-to-wake mode (timestamp will be set to ignore immediate long clicks)
    Buttons::getInstance().setHoldToWakeMode(false);

    return true; // Proceed with full startup
}
