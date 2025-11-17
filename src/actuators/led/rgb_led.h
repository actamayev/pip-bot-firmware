#pragma once

#include <Adafruit_NeoPixel.h>

#include "led_animations.h"
#include "utils/config.h"
#include "utils/structs.h"

constexpr uint8_t ESP_LED_PIN = 4;
constexpr uint8_t NUM_LEDS = 8;

class RgbLed {
  public:
    // Basic color controls
    void turnAllLedsOff();
    void setLedGreen();
    void setLedYellow();
    void setDefaultColors(uint8_t red, uint8_t green, uint8_t blue);

    // Individual LED controls
    void setTopLeftLed(uint8_t red, uint8_t green, uint8_t blue);
    void setTopRightLed(uint8_t red, uint8_t green, uint8_t blue);
    void setMiddleLeftLed(uint8_t red, uint8_t green, uint8_t blue);
    void setMiddleRightLed(uint8_t red, uint8_t green, uint8_t blue);
    void setBackLeftLed(uint8_t red, uint8_t green, uint8_t blue);
    void setBackRightLed(uint8_t red, uint8_t green, uint8_t blue);
    void setMainBoardLedsToColor(uint8_t red, uint8_t green, uint8_t blue);
    void setRightHeadlight(uint8_t red, uint8_t green, uint8_t blue);
    void setLeftHeadlight(uint8_t red, uint8_t green, uint8_t blue);

    // Get current color values
    uint8_t getCurrentRed() const {
        return _currentRed;
    }
    uint8_t getCurrentGreen() const {
        return _currentGreen;
    }
    uint8_t getCurrentBlue() const {
        return _currentBlue;
    }

    // Default colors for each LED (REVERSED ORDER)
    uint8_t defaultColors[8][3] = {
        {0, 0, 0}, // middle_right      - R,G,B
        {0, 0, 0}, // top_right    - R,G,B
        {0, 0, 0}, // right_headlight       - R,G,B
        {0, 0, 0}, // left_headlight - R,G,B
        {0, 0, 0}, // top_left - R,G,B
        {0, 0, 0}, // middle_left      - R,G,B
        {0, 0, 0}, // back_left   - R,G,B
        {0, 0, 0}  // back_right     - R,G,B
    };
    bool defaultColorsSet[8] = {false, false, false, false, false, false, false, false};

    void turnHeadlightsOn();
    void turnHeadlightsOff();
    void turnHeadlightsFaintBlue();

    void turnFrontMiddleLedsFaintBlue();
    void turnFrontMiddleLedsOff();

    void turnBackLedsFaintBlue();
    void turnBackLedsOff();

    void turnMainBoardLedsOff();

  private:
    bool processLedUpdate(int ledIndex, uint8_t red, uint8_t green, uint8_t blue);

    // Current LED state
    uint8_t _currentRed = 0;
    uint8_t _currentGreen = 0;
    uint8_t _currentBlue = 0;

    // TODO: Bring this back later, along with the captureCurrentState and restoreCapturedState functions (see 7/22/25 PR)
    LedState _capturedState{};
};

extern RgbLed rgbLed;
