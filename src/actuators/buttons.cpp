#include "buttons.h"

Buttons::Buttons() 
    : button1(BUTTON_PIN_1), 
      button2(BUTTON_PIN_2) {
    begin();
}

void Buttons::begin() {
    // Configure buttons for pull-down, active HIGH configuration
    button1.begin(BUTTON_PIN_1, INPUT_PULLDOWN, false);
    button2.begin(BUTTON_PIN_2, INPUT_PULLDOWN, false);
    
    button1.setDebounceTime(0);
    button2.setDebounceTime(0);
    
    // Setup deep sleep functionality
    setupDeepSleep();
}

void Buttons::update() {
    // Must be called regularly to process button events
    button1.loop();
    button2.loop();
    // Handle sleep confirmation timeout
    if (waitingForSleepConfirmation && sleepConfirmationStartTime > 0) {
        if (millis() - sleepConfirmationStartTime > SLEEP_CONFIRMATION_TIMEOUT) {
            waitingForSleepConfirmation = false;
            sleepConfirmationStartTime = 0;
            rgbLed.turn_all_leds_off();
            SerialQueueManager::getInstance().queueMessage("Sleep confirmation timed out, returning to normal state");
        }
    }
}

void Buttons::setButton1ClickHandler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;
    
    // Keep pressed handler for pause/resume functionality only
    button1.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        // Check if timeout manager is in confirmation state
        if (TimeoutManager::getInstance().isInConfirmationState()) return;

        // If we're waiting for confirmation, don't process other logic
        if (this->waitingForSleepConfirmation) return;

        BytecodeVM& vm = BytecodeVM::getInstance();

        // Handle pause for running programs only
        if (vm.isPaused == BytecodeVM::RUNNING) {
            vm.pauseProgram();
            this->justPausedOnPress = true;  // Set the flag
            SerialQueueManager::getInstance().queueMessage("Program paused on button press!");
            return;
        }
    });

    // Move program start logic to release handler
    button1.setReleasedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        // Handle deep sleep logic first (existing functionality)
        if (this->longPressFlagForSleep) {
            SerialQueueManager::getInstance().queueMessage("Button released after long press, entering deep sleep confirmation...");
            SerialQueueManager::getInstance().queueMessage("Press Button 1 to confirm sleep or Button 2 to cancel");
            this->longPressFlagForSleep = false;
            this->waitingForSleepConfirmation = true;
            this->sleepConfirmationStartTime = millis();
            return;
        }

        // If we just paused on press, clear the flag and don't resume yet
        if (this->justPausedOnPress) {
            this->justPausedOnPress = false;
            SerialQueueManager::getInstance().queueMessage("Button released after pause - press and release again to resume");
            return;
        }

        // Check if timeout manager is in confirmation state
        if (TimeoutManager::getInstance().isInConfirmationState()) return;

        // If we're waiting for sleep confirmation, don't process program start
        if (this->waitingForSleepConfirmation) return;

        BytecodeVM& vm = BytecodeVM::getInstance();

        // Handle program start on button release
        if (vm.waitingForButtonPressToStart) {
            if (!vm.canStartProgram()) return;
            vm.isPaused = BytecodeVM::RUNNING;
            vm.waitingForButtonPressToStart = false;
            vm.incrementPC();
            SerialQueueManager::getInstance().queueMessage("Program started on button release!");
            return;
        }

        // Handle resume for paused programs (only if we didn't just pause)
        if (vm.isPaused == BytecodeVM::PAUSED) {
            vm.resumeProgram();
            SerialQueueManager::getInstance().queueMessage("Program resumed on button release!");
            return;
        }

        // If no program is running and we're not waiting to start, use original callback
        if (vm.isPaused == BytecodeVM::PROGRAM_NOT_STARTED) {
            SerialQueueManager::getInstance().queueMessage("Program not running - executing normal callback on release");
            if (originalCallback) {
                originalCallback(btn);
            }
        }
    });

    // Keep click handler for sleep confirmation
    button1.setClickHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        if (!(this->waitingForSleepConfirmation)) return;
        SerialQueueManager::getInstance().queueMessage("Sleep confirmed with Button 1! Entering deep sleep...");
        this->waitingForSleepConfirmation = false;
        this->sleepConfirmationStartTime = 0;
        enterDeepSleep();
        return; // Don't call the original callback in this case
    });
}

void Buttons::setButton2ClickHandler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;
    
    button2.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        // Check if timeout manager is in confirmation state
        if (TimeoutManager::getInstance().isInConfirmationState()) return; // Don't process other button logic

        // If we're waiting for confirmation, this click cancels deep sleep
        if (this->waitingForSleepConfirmation) {
            SerialQueueManager::getInstance().queueMessage("Sleep canceled with Button 2!");
            rgbLed.turn_all_leds_off();
            this->waitingForSleepConfirmation = false;
            this->sleepConfirmationStartTime = 0;
            return; // Don't call the original callback in this case
        }
        
        // Otherwise, proceed with normal click handling
        if (originalCallback) {
            originalCallback(btn);
        }
    });
}

void Buttons::setButton1LongPressHandler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        if (callback) callback(btn);
    };
    button1.setLongClickHandler(wrappedCallback);
}

void Buttons::setButton2LongPressHandler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        if (callback) callback(btn);
    };
    button2.setLongClickHandler(wrappedCallback);
}

void Buttons::setupDeepSleep() {
    // First, detect when long press threshold is reached
    button1.setLongClickTime(DEEP_SLEEP_TIMEOUT);
    button1.setLongClickDetectedHandler([this](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        SerialQueueManager::getInstance().queueMessage("Long press detected on Button 1! Release to enter confirmation stage...");
        BytecodeVM::getInstance().pauseProgram();
        rgbLed.set_led_yellow();
        this->longPressFlagForSleep = true;
    });

    // Note: Wakeup detection is now handled in checkHoldToWakeCondition()
}

void Buttons::enterDeepSleep() {
    // Configure both buttons as wake-up sources
    rgbLed.turn_all_leds_off();
    WebSocketManager::getInstance().sendPipTurningOff();
    Speaker::getInstance().setMuted(true);
    BytecodeVM::getInstance().stopProgram();
    DisplayScreen::getInstance().turnScreenOff();
    digitalWrite(PWR_EN, LOW);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Create bitmask for both button pins
    // Bit positions correspond to GPIO numbers
    uint64_t wakeup_pin_mask = (1ULL << BUTTON_PIN_1) | (1ULL << BUTTON_PIN_2);
    
    // Wake up when ANY of the pins goes HIGH (button pressed with pull-down)
    esp_sleep_enable_ext1_wakeup(wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    SerialQueueManager::getInstance().queueMessage("Going to deep sleep now (both buttons can wake)");
    Serial.flush();
    
    esp_deep_sleep_start();
    
    SerialQueueManager::getInstance().queueMessage("This will never be printed");
}
