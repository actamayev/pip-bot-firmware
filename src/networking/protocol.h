#pragma once

#include <Arduino.h>

enum class DataMessageType : uint8_t {
    FIRMWARE_CHUNK = 0,
    MOTOR_CONTROL = 1,
    SOUND_COMMAND = 2,
    SPEAKER_MUTE = 3,
    BALANCE_CONTROL = 4,
    UPDATE_BALANCE_PIDS = 5
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
};

