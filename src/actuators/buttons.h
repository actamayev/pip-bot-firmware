#pragma once
#include <Arduino.h>
#include <Button2.h>
#include <esp_sleep.h>  // Add this for deep sleep functionality
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
        
        // Initialize buttons - call this in setup()
        void begin();
        
        // Update buttons - call this in loop()
        void update();
        
        // Set callbacks for different button events
        void setButton1ClickHandler(std::function<void(Button2&)> callback);
        void setButton2ClickHandler(std::function<void(Button2&)> callback);
        
        // Add more event handlers as needed:
        void setButton1LongPressHandler(std::function<void(Button2&)> callback);
        void setButton2LongPressHandler(std::function<void(Button2&)> callback);
    private:
        Button2 button1;
        Button2 button2;
		bool longPressFlagForSleep = false;
        bool waitingForSleepConfirmation = false; // New flag for confirmation stage
        static const uint32_t DEEP_SLEEP_TIMEOUT = 2000; // 2 seconds in milliseconds
        // Deep sleep methods
        void setupDeepSleep();
        void enterDeepSleep();
};
