#include "buttons.h"

Buttons::Buttons() 
    : button1(BUTTON_PIN_1), 
      button2(BUTTON_PIN_2) {
}

void Buttons::begin() {
    // For ESP32/ESP8266, the Button2 library uses 
    // constructor or begin() method to configure button behavior
    // Default is INPUT_PULLUP with activeLow=true
    
    button1.setDebounceTime(0);
    button2.setDebounceTime(0);
    
    // Setup deep sleep functionality
    setupDeepSleep();
}

void Buttons::update() {
    // Must be called regularly to process button events
    button1.loop();
    button2.loop();
}

void Buttons::setButton1ClickHandler(std::function<void(Button2&)> callback) {
    // Store the user's callback to call it after our internal handling
    auto originalCallback = callback;
    
    button1.setPressedHandler([this, originalCallback](Button2& btn) {
        // If we're waiting for confirmation, this click confirms deep sleep
        if (this->waitingForSleepConfirmation) return;
        
        // Otherwise, proceed with normal click handling
        if (originalCallback) {
            originalCallback(btn);
        }
    });

	button1.setClickHandler([this, originalCallback](Button2& btn) {
        if (!(this->waitingForSleepConfirmation)) return;
		Serial.println("Sleep confirmed with Button 1! Entering deep sleep...");
		this->waitingForSleepConfirmation = false;
		vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to allow serial message to be sent
		enterDeepSleep();
		return; // Don't call the original callback in this case
    });
}

void Buttons::setButton2ClickHandler(std::function<void(Button2&)> callback) {
    // Store the user's callback to call it after our internal handling
    auto originalCallback = callback;
    
    button2.setPressedHandler([this, originalCallback](Button2& btn) {
        // If we're waiting for confirmation, this click cancels deep sleep
        if (this->waitingForSleepConfirmation) {
            Serial.println("Sleep canceled with Button 2!");
            this->waitingForSleepConfirmation = false;
            rgbLed.restoreCapturedState();
            return; // Don't call the original callback in this case
        }
        
        // Otherwise, proceed with normal click handling
        if (originalCallback) {
            originalCallback(btn);
        }
    });
}

void Buttons::setButton1LongPressHandler(std::function<void(Button2&)> callback) {
    button1.setLongClickHandler(callback);
}

void Buttons::setButton2LongPressHandler(std::function<void(Button2&)> callback) {
    button2.setLongClickHandler(callback);
}

void Buttons::setupDeepSleep() {
    // First, detect when long press threshold is reached
    button1.setLongClickTime(DEEP_SLEEP_TIMEOUT);
    button1.setLongClickDetectedHandler([this](Button2& btn) {
        Serial.println("Long press detected on Button 1! Release to enter confirmation stage...");
        BytecodeVM::getInstance().stopProgram();
        rgbLed.captureCurrentState();
        rgbLed.set_led_yellow();
        this->longPressFlagForSleep = true;
    });
    
    // Then, detect when button is released after long press
    button1.setReleasedHandler([this](Button2& btn) {
        if (!(this->longPressFlagForSleep)) return;
        Serial.println("Button released after long press, entering deep sleep confirmation...");
        Serial.println("Press Button 1 to confirm sleep or Button 2 to cancel");
        this->longPressFlagForSleep = false;
        this->waitingForSleepConfirmation = true; // Enter confirmation stage instead of sleeping directly
    });

    // Check if we woke up from deep sleep due to button press
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke up from deep sleep due to button press on GPIO 12");
    }
}

void Buttons::enterDeepSleep() {
    // Configure Button 1 (GPIO 12) as wake-up source
    rgbLed.turn_led_off();
    speaker.mute();
    // MultizoneTofSensor::getInstance().turnOffSensor();
    BytecodeVM::getInstance().stopProgram();
    SideTofManager::getInstance().turnOffSideTofs();

    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN_1, LOW); // LOW = button press (since using INPUT_PULLUP)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN_2, LOW); // LOW = button press (since using INPUT_PULLUP)
    
    Serial.println("Going to deep sleep now");
    Serial.flush();
    
    esp_deep_sleep_start();
    
    Serial.println("This will never be printed");
}
