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
    button1.setDebounceTime(50);
    button2.setDebounceTime(50);
}

void Buttons::update() {
    // Must be called regularly to process button events
    button1.loop();
    button2.loop();
}

void Buttons::setButton1ClickHandler(std::function<void(Button2&)> callback) {
    button1.setClickHandler(callback);
}

void Buttons::setButton2ClickHandler(std::function<void(Button2&)> callback) {
    button2.setClickHandler(callback);
}

void Buttons::setButton1LongPressHandler(std::function<void(Button2&)> callback) {
    button1.setLongClickHandler(callback);
}

void Buttons::setButton2LongPressHandler(std::function<void(Button2&)> callback) {
    button2.setLongClickHandler(callback);
}
