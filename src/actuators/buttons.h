#pragma once
#include <Arduino.h>

#include <Button2.h>
#include <esp_sleep.h>

#include "actuators/led/rgb_led.h"
#include "actuators/speaker.h"
#include "custom_interpreter/bytecode_vm.h"
#include "sensors/side_tof_manager.h"
#include "utils/config.h"
#include "utils/singleton.h"

class Buttons : public Singleton<Buttons> {
    friend class Singleton<Buttons>;

  public:
    Buttons();

    // Update buttons - call this in loop()
    void update();

    // Set callbacks for different button events
    void set_left_button_click_handler(std::function<void(Button2&)> callback);
    void set_right_button_click_handler(std::function<void(Button2&)> callback);

    // Add more event handlers as needed:
    void set_left_button_long_press_handler(std::function<void(Button2&)> callback);
    void set_right_button_long_press_handler(std::function<void(Button2&)> callback);
    static void enter_deep_sleep();

    // Hold-to-wake mode control
    void set_hold_to_wake_mode(bool enabled);

    // Check if either button is currently pressed
    bool is_either_button_pressed();

    // Check if specific button is currently pressed
    bool is_right_button_pressed();

  private:
    Button2 _left_button;
    Button2 _right_button;
    bool _long_press_flag_for_sleep = false;
    bool _just_paused_on_press = false;
    bool _waiting_for_sleep_confirmation = false;    // New flag for confirmation stage
    bool _in_hold_to_wake_mode = false;              // Flag to prevent long click LED during hold-to-wake
    uint32_t _hold_to_wake_completed_time = 0;       // Timestamp when hold-to-wake completed
    static const uint32_t DEEP_SLEEP_TIMEOUT = 1000; // 1 second in milliseconds
    // Add for sleep confirmation timeout
    static const uint32_t SLEEP_CONFIRMATION_TIMEOUT = 5000; // 5 seconds in milliseconds
    uint32_t _sleep_confirmation_start_time = 0;

    void begin();

    // Deep sleep methods
    void setup_deep_sleep();
};
