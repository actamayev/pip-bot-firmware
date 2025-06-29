#include "hold_to_wake.h"

// Function to check if we should proceed with full startup after deep sleep wake
bool holdToWake() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    // Only apply hold-to-wake behavior if woken from deep sleep by button
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0) {
        rgbLed.setDefaultColors(0, 0, MAX_LED_BRIGHTNESS);
        ledAnimations.startBreathing(2000, 0.0f);
        return true; // Normal startup for other wake reasons
    }
    
    SerialQueueManager::getInstance().queueMessage("Woke up from deep sleep due to button press. Checking hold duration...");
    
    // Configure button pin for reading
    pinMode(BUTTON_PIN_1, INPUT_PULLUP);
    
    // Check if button is still pressed (LOW due to INPUT_PULLUP)
    if (digitalRead(BUTTON_PIN_1) == HIGH) {
        SerialQueueManager::getInstance().queueMessage("Button already released. Going back to sleep...");
        Buttons::getInstance().enterDeepSleep();
        return false;
    }
    
    // Start timing - button must be held for 1000ms
    const uint32_t HOLD_DURATION_MS = 1000;
    uint32_t startTime = millis();
    
    while ((millis() - startTime) < HOLD_DURATION_MS) {
        if (digitalRead(BUTTON_PIN_1) == HIGH) {
            uint32_t heldTime = millis() - startTime;
            String message = "Button released after " + String(heldTime) + "ms. Going back to sleep...";
            SerialQueueManager::getInstance().queueMessage(message.c_str());
            Buttons::getInstance().enterDeepSleep();
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // WAKE-UP SUCCESSFUL - Reset button state so this press doesn't count toward shutdown
    SerialQueueManager::getInstance().queueMessage("Wake-up successful - clearing button state");
    rgbLed.setDefaultColors(0, 0, MAX_LED_BRIGHTNESS);
    ledAnimations.startBreathing(2000, 0.0f);

    // Wait for user to release the button before proceeding
    while (digitalRead(BUTTON_PIN_1) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    SerialQueueManager::getInstance().queueMessage("Button released - ready for normal operation");
    
    return true; // Proceed with full startup
}
