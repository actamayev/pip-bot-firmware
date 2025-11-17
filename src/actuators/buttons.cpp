#include "buttons.h"

#include "games/game_manager.h"

Buttons::Buttons() : _left_button(LEFT_BUTTON_PIN), _right_button(RIGHT_BUTTON_PIN) {
    begin();
}

void Buttons::begin() {
    // Configure buttons for pull-down, active HIGH configuration
    _left_button.begin(LEFT_BUTTON_PIN, INPUT_PULLDOWN, false);
    _right_button.begin(RIGHT_BUTTON_PIN, INPUT_PULLDOWN, false);

    _left_button.setDebounceTime(0);
    _right_button.setDebounceTime(0);

    // Setup deep sleep functionality
    setup_deep_sleep();
}

void Buttons::update() {
    // Must be called regularly to process button events
    _left_button.loop();
    _right_button.loop();
    // Handle sleep confirmation timeout
    if (_waiting_for_sleep_confirmation && _sleep_confirmation_start_time > 0) {
        if (millis() - _sleep_confirmation_start_time > SLEEP_CONFIRMATION_TIMEOUT) {
            _waiting_for_sleep_confirmation = false;
            _sleep_confirmation_start_time = 0;
            rgb_led.turn_all_leds_off();
        }
    }
}

void Buttons::set_left_button_click_handler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;

    _left_button.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().reset_activity();

        BytecodeVM& vm = BytecodeVM::get_instance();
        // Handle resume for paused programs (only if we didn't just pause)
        if (vm._isPaused == BytecodeVM::RUNNING || vm._isPaused == BytecodeVM::PROGRAM_FINISHED) {
            vm.pause_program();
            this->_just_paused_on_press = true;
            return;
        }
    });

    // Move program start logic to release handler
    _left_button.setReleasedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().reset_activity();

        // Handle deep sleep logic first (existing functionality)
        if (this->_long_press_flag_for_sleep) {
            this->_long_press_flag_for_sleep = false;
            this->_waiting_for_sleep_confirmation = true;
            this->_sleep_confirmation_start_time = millis();
            return;
        }

        // Check if timeout manager is in confirmation state
        if (TimeoutManager::get_instance().is_in_confirmation_state()) return;

        // If we're waiting for sleep confirmation, don't process program start
        if (this->_waiting_for_sleep_confirmation) return;

        BytecodeVM& vm = BytecodeVM::get_instance();

        // Handle program start on button release
        if (vm.waitingForButtonPressToStart) {
            if (this->_just_paused_on_press) {
                this->_just_paused_on_press = false;
                return; // Skip start on same press/release cycle
            }
            if (!vm.can_start_program()) return;
            vm._isPaused = BytecodeVM::RUNNING;
            vm.waitingForButtonPressToStart = false;
            vm.increment_pc();
            return;
        }

        // Handle resume for paused programs (only if we didn't just pause)
        if (vm._isPaused == BytecodeVM::PAUSED) {
            if (this->_just_paused_on_press) {
                this->_just_paused_on_press = false;
                return; // Skip resume on same press/release cycle
            }
            vm.resume_program();
            return;
        }

        // Handle restart for finished programs
        if (vm._isPaused == BytecodeVM::PROGRAM_FINISHED) {
            vm.resume_program(); // This will restart from beginning
            return;
        }

        // If no program is running and we're not waiting to start, use original callback
        if (vm._isPaused == BytecodeVM::PROGRAM_NOT_STARTED) {
            if (originalCallback) {
                originalCallback(btn);
            }
        }
    });

    // Keep click handler for sleep confirmation
    _left_button.setClickHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().reset_activity();

        if (!(this->_waiting_for_sleep_confirmation)) return;
        this->_waiting_for_sleep_confirmation = false;
        this->_sleep_confirmation_start_time = 0;
        enter_deep_sleep();
        return; // Don't call the original callback in this case
    });
}

void Buttons::set_right_button_click_handler(std::function<void(Button2&)> callback) {
    auto originalCallback = callback;

    _right_button.setPressedHandler([this, originalCallback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().reset_activity();

        // Check if timeout manager is in confirmation state
        if (TimeoutManager::get_instance().is_in_confirmation_state()) return; // Don't process other button logic

        // If we're waiting for confirmation, this click cancels deep sleep
        if (this->_waiting_for_sleep_confirmation) {
            rgb_led.turn_all_leds_off();
            this->_waiting_for_sleep_confirmation = false;
            this->_sleep_confirmation_start_time = 0;
            return; // Don't call the original callback in this case
        }

        // TODO 10/3: Figure out the right way to handle this. We did this to be able to check if (is_right_button_pressed) in bytecode_interpreter
        // BytecodeVM& vm = BytecodeVM::get_instance();

        // // Handle pause for running programs
        // if (vm._isPaused == BytecodeVM::RUNNING || vm._isPaused == BytecodeVM::PROGRAM_FINISHED) {
        //     vm.pause_program();
        //     return;
        // }

        // Otherwise, proceed with normal click handling - check games first
        if (GameManager::get_instance().is_any_game_active()) {
            GameManager::get_instance().handle_button_press(true); // Right button pressed
        } else if (originalCallback) {
            originalCallback(btn);
        }
    });
}

void Buttons::set_left_button_long_press_handler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().reset_activity();
        if (callback) callback(btn);
    };
    _left_button.setLongClickHandler(wrappedCallback);
}

void Buttons::set_right_button_long_press_handler(std::function<void(Button2&)> callback) {
    auto wrappedCallback = [callback](Button2& btn) {
        // Reset timeout on any button activity
        TimeoutManager::get_instance().reset_activity();
        if (callback) callback(btn);
    };
    _right_button.setLongClickHandler(wrappedCallback);
}

void Buttons::setup_deep_sleep() {
    // First, detect when long press threshold is reached
    _left_button.setLongClickTime(DEEP_SLEEP_TIMEOUT);
    _left_button.setLongClickDetectedHandler([this](Button2& btn) {
        if (this->_in_hold_to_wake_mode) {
            return;
        }

        // Ignore long clicks that happen within 500ms of hold-to-wake completion
        if (this->_hold_to_wake_completed_time > 0 && (millis() - this->_hold_to_wake_completed_time) < 500) {
            return;
        }

        // Reset timeout on any button activity
        TimeoutManager::get_instance().reset_activity();

        BytecodeVM::get_instance().pause_program();
        rgb_led.set_led_yellow();
        this->_long_press_flag_for_sleep = true;
    });

    // Note: Wakeup detection is now handled in checkHoldToWakeCondition()
}

void Buttons::enter_deep_sleep() {
    if (SerialManager::get_instance().is_serial_connected()) {
        SerialManager::get_instance().send_pip_turning_off();
    } else if (WebSocketManager::get_instance().is_ws_connected()) {
        WebSocketManager::get_instance().send_pip_turning_off();
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
    this->_in_hold_to_wake_mode = enabled;
    if (enabled) {
        return;
    }
    this->_hold_to_wake_completed_time = millis();
}

bool Buttons::is_either_button_pressed() {
    return _left_button.isPressed() || _right_button.isPressed();
}

bool Buttons::is_right_button_pressed() {
    return _right_button.isPressed();
}
