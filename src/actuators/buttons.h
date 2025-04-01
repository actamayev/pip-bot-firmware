#pragma once
#include <Arduino.h>
#include <Button2.h>
#include "../utils/config.h"
#include "../utils/singleton.h"

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
};
