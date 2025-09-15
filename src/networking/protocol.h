#pragma once

enum class DataMessageType : uint8_t {
    UPDATE_AVAILABLE = 0,
    MOTOR_CONTROL = 1,
    SOUND_COMMAND = 2,
    SPEAKER_MUTE = 3,
    BALANCE_CONTROL = 4,
    UPDATE_BALANCE_PIDS = 5,
    UPDATE_LIGHT_ANIMATION = 6,
    UPDATE_LED_COLORS = 7,
    BYTECODE_PROGRAM = 8,
    STOP_SANDBOX_CODE = 9,
    OBSTACLE_AVOIDANCE = 10,
    SERIAL_HANDSHAKE = 11,
    SERIAL_KEEPALIVE = 12,
    SERIAL_END = 13,
    UPDATE_HEADLIGHTS = 14,
    START_SENSOR_POLLING = 15,
    WIFI_CREDENTIALS = 16,
    WIFI_CONNECTION_RESULT = 17,
    GET_SAVED_WIFI_NETWORKS = 18,
    SOFT_SCAN_WIFI_NETWORKS = 19,
    HARD_SCAN_WIFI_NETWORKS = 20, // This overrides the cache (will force a scan, even if there are available networks)
    UPDATE_HORN_SOUND = 21,
    SPEAKER_VOLUME = 22,
    STOP_SOUND = 23, // For stopping ongoing sounds (e.g. horn sound)
    REQUEST_BATTERY_MONITOR_DATA = 24,
    UPDATE_DISPLAY = 25,
    STOP_SENSOR_POLLING = 26,
    TRIGGER_MESSAGE = 27,
    STOP_CAREER_QUEST_TRIGGER = 28,
    SHOW_DISPLAY_START_SCREEN = 29
};

// Sound types
enum class SoundType : uint8_t {
    CHIME = 1,
    CHIRP = 2,
    POP = 3,
    DROP = 4,
    FART = 5,
    MONKEY = 6,
    ELEPHANT = 7,
    PARTY = 8,
    UFO = 9,
    COUNTDOWN = 10,
    ENGINE = 11,
    ROBOT = 12
};

// Speaker status
enum class SpeakerStatus : uint8_t {
    UNMUTED = 0,
    MUTED = 1
};

enum class HeadlightStatus {
    OFF = 0,
    ON = 1
};

enum class HornSoundStatus {
    OFF = 0,
    ON = 1
};

enum class BalanceStatus : uint8_t {
    UNBALANCED = 0,
    BALANCED = 1
};

enum class ObstacleAvoidanceStatus : uint8_t {
    STOP_AVOIDANCE = 0,
    AVOID = 1
};

enum class LightAnimationStatus : uint8_t {
    NO_ANIMATION = 0,
    BREATHING = 1,
    RAINBOW = 2,
    STROBE = 3,
    TURN_OFF = 4,
    FADE_OUT = 5
};

enum class WiFiConnectionStatus : uint8_t {
    FAILED = 0,
    WIFI_ONLY = 1,
    WIFI_AND_WEBSOCKET_SUCCESS = 2
};

enum class CareerType : uint8_t {
    MEET_PIP = 1
};

enum class MeetPipTriggerType : uint8_t {
    ENTER_CAREER = 0,
    S2_P1_ENTER = 1,
    S2_P1_EXIT = 2,
    S2_P4_ENTER = 3,
    S2_P4_EXIT = 4,
    S3_P3_ENTER = 5,
    S3_P3_EXIT = 6,
    S4_P5_ENTER = 7,
    S5_P4_ENTER = 8,
    S5_P4_EXIT = 9,
    S5_P5_ENTER = 10,
    S5_P5_EXIT = 11,
    S6_P4_ENTER = 12,
    S6_P4_EXIT = 13,
    S6_P6_ENTER = 14,
    S6_P6_EXIT = 15,
    S7_P4_ENTER = 16,
    S7_P4_EXIT = 17,
    S7_P6_ENTER = 18,
    S7_P6_EXIT = 19,
    S8_P3_ENTER = 20,
    S8_P3_EXIT = 21,
    S9_P3_ENTER = 22,
    S9_P6_ENTER = 23,
    S9_P6_EXIT = 24,
    S4_P5_EXIT = 25,
    S9_P3_EXIT = 26,
    S4_P4_EXIT = 27,
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

    uint8_t rightHeadlightRed;
    uint8_t rightHeadlightGreen;
    uint8_t rightHeadlightBlue;

    uint8_t leftHeadlightRed;
    uint8_t leftHeadlightGreen;
    uint8_t leftHeadlightBlue;
};

// Markers for serial communication
const uint8_t START_MARKER = 0xAA;
const uint8_t END_MARKER = 0x55;
