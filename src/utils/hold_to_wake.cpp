#include "hold_to_wake.h"
#include "utils/task_manager.h"  // Add this include for TaskManager

// Function to check if we should proceed with full startup after deep sleep wake
bool holdToWake() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    // Only apply hold-to-wake behavior if woken from deep sleep by button
    // Now using EXT1 instead of EXT0
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT1) {
        rgbLed.setDefaultColors(0, 0, MAX_LED_BRIGHTNESS);
        ledAnimations.startBreathing(2000, 0.0f);
        return true; // Normal startup for other wake reasons
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
    
    // HOLD DURATION REACHED - INITIALIZE DISPLAY IMMEDIATELY
    // Set up minimal required components for display
    initializeEarlyComponents();
    
    // Initialize display immediately when hold duration is met (only if not already done)
    if (!TaskManager::isDisplayInitialized()) {
        TaskManager::createDisplayInitTask();
    }
    
    // Give the display task a moment to start
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // WAKE-UP SUCCESSFUL - Reset button state so this press doesn't count toward shutdown
    rgbLed.setDefaultColors(0, 0, MAX_LED_BRIGHTNESS);
    ledAnimations.startBreathing(2000, 0.0f);

    // Wait for user to release all buttons before proceeding
    do {
        button1Pressed = digitalRead(BUTTON_PIN_1) == HIGH;
        button2Pressed = digitalRead(BUTTON_PIN_2) == HIGH;
        vTaskDelay(pdMS_TO_TICKS(10));
    } while (button1Pressed || button2Pressed);
    
    return true; // Proceed with full startup
}

// NEW FUNCTION: Initialize minimal components needed for display
void initializeEarlyComponents() {
    static bool initialized = false;
    if (initialized) return; // Avoid double-initialization
    
    // Initialize Serial Queue Manager first (needed for logging)
    SerialQueueManager::getInstance().initialize();
    TaskManager::createSerialQueueTask();
    
    // Initialize I2C (needed for display)
    Wire.setPins(I2C_SDA_1, I2C_SCL_1);
    Wire.begin(I2C_SDA_1, I2C_SCL_1, I2C_CLOCK_SPEED);

    Wire1.setPins(I2C_SDA_2, I2C_SCL_2);
    Wire1.begin(I2C_SDA_2, I2C_SCL_2, I2C_CLOCK_SPEED);
    
    // Small delay to ensure I2C is ready
    vTaskDelay(pdMS_TO_TICKS(10));
    
    initialized = true;
}
