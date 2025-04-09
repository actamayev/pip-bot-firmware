#pragma once

#include <Arduino.h>

enum class DataMessageType : uint8_t {
    FIRMWARE_CHUNK = 0,
    MOTOR_CONTROL = 1,
    SOUND_COMMAND = 2,
    SPEAKER_MUTE = 3,
    BALANCE_CONTROL = 4,
    UPDATE_BALANCE_PIDS = 5,
    UPDATE_LIGHT_ANIMATION = 6,
    UPDATE_LED_COLORS = 7
};

// Sound types
enum class SoundType : uint8_t {
    ALERT = 0,
    BEEP = 1,
    CHIME = 2
};

// Speaker status
enum class SpeakerStatus : uint8_t {
    UNMUTED = 0,
    MUTED = 1
};

enum class BalanceStatus : uint8_t {
    UNBALANCED = 0,
    BALANCED = 1
};

enum class LightAnimationStatus : uint8_t {
    NO_ANIMATION = 0,
    BREATHING = 1,
    RAINBOW = 2,
    STROBE = 3,
    TURN_OFF = 4,
    FADE_OUT = 5
};

struct NewBalancePids {
    float pValue;               // 4 bytes
    float iValue;               // 4 bytes
    float dValue;               // 4 bytes
    float ffValue;              // 4 bytes
    float targetAngle;          // 4 bytes
    float maxSafeAngleDeviation; // 4 bytes
    float updateInterval;       // 4 bytes
    float deadbandAngle;        // 4 bytes
    float maxStableRotation;    // 4 bytes
    float minEffectivePwm;    // 4 bytes
};

struct NewLightColors { 
    uint8_t topLeftRed;
    uint8_t topLeftGreen;
    uint8_t topLeftBlue;
    
    uint8_t topRightRed;
    uint8_t topRightGreen;
    uint8_t topRightBlue;
    
    uint8_t middleLeftRed;
    uint8_t middleLeftGreen;
    uint8_t middleLeftBlue;
    
    uint8_t middleRightRed;
    uint8_t middleRightGreen;
    uint8_t middleRightBlue;
    
    uint8_t backLeftRed;
    uint8_t backLeftGreen;
    uint8_t backLeftBlue;
    
    uint8_t backRightRed;
    uint8_t backRightGreen;
    uint8_t backRightBlue;
};

