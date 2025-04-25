#include "./buttons.h"

Buttons::Buttons() 
    : button1(BUTTON_PIN_1), 
      button2(BUTTON_PIN_2) {
}

void Buttons::begin() {
    // For ESP32/ESP8266, the Button2 library uses 
    // constructor or begin() method to configure button behavior
    // Default is INPUT_PULLUP with activeLow=true
    
    // Set debounce time if needed (default is 50ms)
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
    button1.setPressedHandler(callback);
}

void Buttons::setButton2ClickHandler(std::function<void(Button2&)> callback) {
    button2.setPressedHandler(callback);
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
        Serial.println("Long press detected on Button 1! Release to enter deep sleep...");
        rgbLed.set_led_yellow();
        this->longPressFlagForSleep = true;
    });
    
    // Then, detect when button is released after long press
    button1.setReleasedHandler([this](Button2& btn) {
        if (this->longPressFlagForSleep) {
            Serial.println("Button released after long press, entering deep sleep...");
            rgbLed.turn_led_off();
            delay(100); // Small delay to allow serial message to be sent
            this->longPressFlagForSleep = false;
            enterDeepSleep();
        }
    });
    
    // Check if we woke up from deep sleep due to button press
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke up from deep sleep due to button press on GPIO 12");
    }
}

void Buttons::enterDeepSleep() {
    // Configure Button 1 (GPIO 12) as wake-up source
    // RTC GPIO number corresponds to the GPIO number
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN_1, LOW); // LOW = button press (since using INPUT_PULLUP)
    
    // Optional: Print a message before going to sleep
    Serial.println("Going to deep sleep now");
    Serial.flush();
    
    // Enter deep sleep
    esp_deep_sleep_start();
    
    // Code execution will not reach here unless deep sleep fails
    Serial.println("This will never be printed");
}
