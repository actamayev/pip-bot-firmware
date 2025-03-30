#pragma once

#include <Arduino.h>

enum class DataMessageType : uint8_t {
    FIRMWARE_CHUNK = 0,
    MOTOR_CONTROL = 1,
    SOUND_COMMAND = 2,
    SPEAKER_MUTE = 3
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
