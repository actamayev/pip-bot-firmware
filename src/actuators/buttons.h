#pragma once
#include <Arduino.h>
#include <Button2.h>
#include <esp_sleep.h>
#include "utils/config.h"
#include "utils/singleton.h"
#include "actuators/speaker.h"
#include "actuators/led/rgb_led.h"
#include "sensors/side_tof_manager.h"
#include "custom_interpreter/bytecode_vm.h"

class Buttons: public Singleton<Buttons> {
    friend class Singleton<Buttons>;
    
    public:
        Buttons();
        
        // Update buttons - call this in loop()
        void update();
        
        // Set callbacks for different button events
        void setLeftButtonClickHandler(std::function<void(Button2&)> callback);
        void setRightButtonClickHandler(std::function<void(Button2&)> callback);
        
        // Add more event handlers as needed:
        void setLeftButtonLongPressHandler(std::function<void(Button2&)> callback);
        void setRightButtonLongPressHandler(std::function<void(Button2&)> callback);
        void enterDeepSleep();
        
        // Hold-to-wake mode control
        void setHoldToWakeMode(bool enabled);
        
        // Check if either button is currently pressed
        bool isEitherButtonPressed();

        // Check if specific button is currently pressed
        bool isRightButtonPressed();

    private:
        Button2 leftButton;
        Button2 rightButton;
		bool longPressFlagForSleep = false;
        bool justPausedOnPress = false;
        bool waitingForSleepConfirmation = false; // New flag for confirmation stage
        bool inHoldToWakeMode = false; // Flag to prevent long click LED during hold-to-wake
        unsigned long holdToWakeCompletedTime = 0; // Timestamp when hold-to-wake completed
        static const uint32_t DEEP_SLEEP_TIMEOUT = 1000; // 1 second in milliseconds
        // Add for sleep confirmation timeout
        static const uint32_t SLEEP_CONFIRMATION_TIMEOUT = 5000; // 5 seconds in milliseconds
        unsigned long sleepConfirmationStartTime = 0;

        void begin();

        // Deep sleep methods
        void setupDeepSleep();
};
