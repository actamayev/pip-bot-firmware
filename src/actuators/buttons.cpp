#include "buttons.h"

#include "games/game_manager.h"

Buttons::Buttons() : leftButton(LEFT_BUTTON_PIN), rightButton(RIGHT_BUTTON_PIN) {
    begin();
}

void Buttons::begin() {
    // Configure buttons for pull-down, active HIGH configuration
    leftButton.begin(LEFT_BUTTON_PIN, INPUT_PULLDOWN, false);
    rightButton.begin(RIGHT_BUTTON_PIN, INPUT_PULLDOWN, false);

    leftButton.setDebounceTime(0);
    rightButton.setDebounceTime(0);

    // Setup deep sleep functionality
    setup_deep_sleep();
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

void Buttons::set_left_button_click_handler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;

    leftButton.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().resetActivity();

        BytecodeVM& vm = BytecodeVM::get_instance();
        // Handle resume for paused programs (only if we didn't just pause)
        if (vm.isPaused == BytecodeVM::RUNNING || vm.isPaused == BytecodeVM::PROGRAM_FINISHED) {
            vm.pauseProgram();
            this->justPausedOnPress = true;
            return;
        }
    });

    // Move program start logic to release handler
    leftButton.setReleasedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().resetActivity();

        // Handle deep sleep logic first (existing functionality)
        if (this->longPressFlagForSleep) {
            this->longPressFlagForSleep = false;
            this->waitingForSleepConfirmation = true;
            this->sleepConfirmationStartTime = millis();
            return;
        }

        // Check if timeout manager is in confirmation state
        if (TimeoutManager::get_instance().isInConfirmationState()) return;

        // If we're waiting for sleep confirmation, don't process program start
        if (this->waitingForSleepConfirmation) return;

        BytecodeVM& vm = BytecodeVM::get_instance();

        // Handle program start on button release
        if (vm.waitingForButtonPressToStart) {
            if (this->justPausedOnPress) {
                this->justPausedOnPress = false;
                return; // Skip start on same press/release cycle
            }
            if (!vm.canStartProgram()) return;
            vm.isPaused = BytecodeVM::RUNNING;
            vm.waitingForButtonPressToStart = false;
            vm.incrementPC();
            return;
        }

        // Handle resume for paused programs (only if we didn't just pause)
        if (vm.isPaused == BytecodeVM::PAUSED) {
            if (this->justPausedOnPress) {
                this->justPausedOnPress = false;
                return; // Skip resume on same press/release cycle
            }
            vm.resumeProgram();
            return;
        }

        // Handle restart for finished programs
        if (vm.isPaused == BytecodeVM::PROGRAM_FINISHED) {
            vm.resumeProgram(); // This will restart from beginning
            return;
        }

        // If no program is running and we're not waiting to start, use original callback
        if (vm.isPaused == BytecodeVM::PROGRAM_NOT_STARTED) {
            if (originalCallback) {
                originalCallback(btn);
            }
        }
    });

    // Keep click handler for sleep confirmation
    leftButton.setClickHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().resetActivity();

        if (!(this->waitingForSleepConfirmation)) return;
        this->waitingForSleepConfirmation = false;
        this->sleepConfirmationStartTime = 0;
        enter_deep_sleep();
        return; // Don't call the original callback in this case
    });
}

void Buttons::set_right_button_click_handler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;

    rightButton.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().resetActivity();

        // Check if timeout manager is in confirmation state
        if (TimeoutManager::get_instance().isInConfirmationState()) return; // Don't process other button logic

        // If we're waiting for confirmation, this click cancels deep sleep
        if (this->waitingForSleepConfirmation) {
            rgbLed.turn_all_leds_off();
            this->waitingForSleepConfirmation = false;
            this->sleepConfirmationStartTime = 0;
            return; // Don't call the original callback in this case
        }

        // TODO 10/3: Figure out the right way to handle this. We did this to be able to check if (is_right_button_pressed) in bytecode_interpreter
        // BytecodeVM& vm = BytecodeVM::get_instance();

        // // Handle pause for running programs
        // if (vm.isPaused == BytecodeVM::RUNNING || vm.isPaused == BytecodeVM::PROGRAM_FINISHED) {
        //     vm.pauseProgram();
        //     return;
        // }

        // Otherwise, proceed with normal click handling - check games first
        if (GameManager::get_instance().isAnyGameActive()) {
            GameManager::get_instance().handleButtonPress(true); // Right button pressed
        } else if (originalCallback) {
            originalCallback(btn);
        }
    });
}

void Buttons::set_left_button_long_press_handler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().resetActivity();
        if (callback) callback(btn);
    };
    leftButton.setLongClickHandler(wrappedCallback);
}

void Buttons::set_right_button_long_press_handler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().resetActivity();
        if (callback) callback(btn);
    };
    rightButton.setLongClickHandler(wrappedCallback);
}

void Buttons::setup_deep_sleep() {
    // First, detect when long press threshold is reached
    leftButton.setLongClickTime(DEEP_SLEEP_TIMEOUT);
    leftButton.setLongClickDetectedHandler([this](Button2& btn) {
        if (this->inHoldToWakeMode) return;

        // Ignore long clicks that happen within 500ms of hold-to-wake completion
        if (this->holdToWakeCompletedTime > 0 && (millis() - this->holdToWakeCompletedTime) < 500) {
            return;
        }

        // Reset timeout on any button activity
        TimeoutManager::get_instance().resetActivity();

        BytecodeVM::get_instance().pauseProgram();
        rgbLed.set_led_yellow();
        this->longPressFlagForSleep = true;
    });

    // Note: Wakeup detection is now handled in checkHoldToWakeCondition()
}

void Buttons::enter_deep_sleep() {
    if (SerialManager::get_instance().isSerialConnected()) {
        SerialManager::get_instance().sendPipTurningOff();
    } else if (WebSocketManager::get_instance().isWsConnected()) {
        WebSocketManager::get_instance().sendPipTurningOff();
    }

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

void Buttons::set_hold_to_wake_mode(bool enabled) {
    this->inHoldToWakeMode = enabled;
    if (enabled) return;
    this->holdToWakeCompletedTime = millis();
}

bool Buttons::is_either_button_pressed() {
    return leftButton.isPressed() || rightButton.isPressed();
}

bool Buttons::is_right_button_pressed() {
    return rightButton.isPressed();
}
