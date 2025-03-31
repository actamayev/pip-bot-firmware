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
    uint8_t pValue;              // Byte 1
    uint8_t iValue;              // Byte 2
    uint8_t dValue;              // Byte 3
    uint8_t ffValue;             // Byte 4
    uint8_t targetAngle;         // Byte 5
    uint8_t maxSafeAngleDeviation; // Byte 6
    uint8_t updateInterval;      // Byte 7
    uint8_t deadbandAngle;       // Byte 8
    uint8_t maxStableRotation;   // Byte 9
};
