#include "buttons.h"
#include "games/game_manager.h"

Buttons::Buttons() 
    : leftButton(LEFT_BUTTON_PIN), 
      rightButton(RIGHT_BUTTON_PIN) {
    begin();
}

void Buttons::begin() {
    // Configure buttons for pull-down, active HIGH configuration
    leftButton.begin(LEFT_BUTTON_PIN, INPUT_PULLDOWN, false);
    rightButton.begin(RIGHT_BUTTON_PIN, INPUT_PULLDOWN, false);
    
    leftButton.setDebounceTime(0);
    rightButton.setDebounceTime(0);
    
    // Setup deep sleep functionality
    setupDeepSleep();
}

void Buttons::update() {
    // Must be called regularly to process button events
    leftButton.loop();
    rightButton.loop();
    // Handle sleep confirmation timeout
    if (waitingForSleepConfirmation && sleepConfirmationStartTime > 0) {
        if (millis() - sleepConfirmationStartTime > SLEEP_CONFIRMATION_TIMEOUT) {
            waitingForSleepConfirmation = false;
            sleepConfirmationStartTime = 0;
            rgbLed.turn_all_leds_off();
        }
    }
}

void Buttons::setLeftButtonClickHandler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;
    
    leftButton.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
    });

    // Move program start logic to release handler
    leftButton.setReleasedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        // Handle deep sleep logic first (existing functionality)
        if (this->longPressFlagForSleep) {
            this->longPressFlagForSleep = false;
            this->waitingForSleepConfirmation = true;
            this->sleepConfirmationStartTime = millis();
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
            return;
        }

        // Handle resume for paused programs (only if we didn't just pause)
        if (vm.isPaused == BytecodeVM::PAUSED) {
            vm.resumeProgram();
            return;
        }
        
        // Handle restart for finished programs
        if (vm.isPaused == BytecodeVM::PROGRAM_FINISHED) {
            vm.resumeProgram(); // This will restart from beginning
            return;
        }

        // If no program is running and we're not waiting to start, check for games first
        if (vm.isPaused == BytecodeVM::PROGRAM_NOT_STARTED) {
            // Forward button press to game manager if game is active
            if (GameManager::getInstance().isAnyGameActive()) {
                GameManager::getInstance().handleButtonPress(true, false); // Left button pressed
            } else if (originalCallback) {
                originalCallback(btn);
            }
        }
    });

    // Keep click handler for sleep confirmation
    leftButton.setClickHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        if (!(this->waitingForSleepConfirmation)) return;
        this->waitingForSleepConfirmation = false;
        this->sleepConfirmationStartTime = 0;
        enterDeepSleep();
        return; // Don't call the original callback in this case
    });
}

void Buttons::setRightButtonClickHandler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;
    
    rightButton.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        // Check if timeout manager is in confirmation state
        if (TimeoutManager::getInstance().isInConfirmationState()) return; // Don't process other button logic

        // If we're waiting for confirmation, this click cancels deep sleep
        if (this->waitingForSleepConfirmation) {
            rgbLed.turn_all_leds_off();
            this->waitingForSleepConfirmation = false;
            this->sleepConfirmationStartTime = 0;
            return; // Don't call the original callback in this case
        }
        
        BytecodeVM& vm = BytecodeVM::getInstance();
        
        // Handle pause for running programs
        if (vm.isPaused == BytecodeVM::RUNNING || vm.isPaused == BytecodeVM::PROGRAM_FINISHED) {
            vm.pauseProgram();
            return;
        }
        
        // Otherwise, proceed with normal click handling - check games first
        if (GameManager::getInstance().isAnyGameActive()) {
            GameManager::getInstance().handleButtonPress(false, true); // Right button pressed
        } else if (originalCallback) {
            originalCallback(btn);
        }
    });
}

void Buttons::setLeftButtonLongPressHandler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        if (callback) callback(btn);
    };
    leftButton.setLongClickHandler(wrappedCallback);
}

void Buttons::setRightButtonLongPressHandler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        if (callback) callback(btn);
    };
    rightButton.setLongClickHandler(wrappedCallback);
}

void Buttons::setupDeepSleep() {
    // First, detect when long press threshold is reached
    leftButton.setLongClickTime(DEEP_SLEEP_TIMEOUT);
    leftButton.setLongClickDetectedHandler([this](Button2& btn) {
        if (this->inHoldToWakeMode) return;
        
        // Ignore long clicks that happen within 500ms of hold-to-wake completion
        if (this->holdToWakeCompletedTime > 0 && (millis() - this->holdToWakeCompletedTime) < 500) {
            return;
        }
        
        // Reset timeout on any button activity
        TimeoutManager::getInstance().resetActivity();
        
        BytecodeVM::getInstance().pauseProgram();
        rgbLed.set_led_yellow();
        this->longPressFlagForSleep = true;
    });

    // Note: Wakeup detection is now handled in checkHoldToWakeCondition()
}

void Buttons::enterDeepSleep() {
    // Configure both buttons as wake-up sources
    WebSocketManager::getInstance().sendPipTurningOff();
    digitalWrite(PWR_EN, LOW);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Create bitmask for both button pins
    // Bit positions correspond to GPIO numbers
    uint64_t wakeup_pin_mask = (1ULL << LEFT_BUTTON_PIN) | (1ULL << RIGHT_BUTTON_PIN);
    
    // Wake up when ANY of the pins goes HIGH (button pressed with pull-down)
    esp_sleep_enable_ext1_wakeup(wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    Serial.flush();
    
    esp_deep_sleep_start();
}

void Buttons::setHoldToWakeMode(bool enabled) {
    this->inHoldToWakeMode = enabled;
    if (enabled) return;
    this->holdToWakeCompletedTime = millis();
}

bool Buttons::isEitherButtonPressed() {
    return leftButton.isPressed() || rightButton.isPressed();
}
