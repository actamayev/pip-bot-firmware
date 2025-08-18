#pragma once
#include <Arduino.h>

struct WiFiCredentials {
	String ssid;
	String password;
};

struct WiFiNetworkInfo {
    String ssid;
    int32_t rssi;
    uint8_t encryptionType;
};

struct EnabledReports {
    bool gameRotationVector = false;
    bool accelerometer = false;
    bool gyroscope = false;
    bool magneticField = false;
};

struct QuaternionData {
    float qX, qY, qZ, qW;
    bool isValid = false;
};

struct EulerAngles {
    float yaw;
    float pitch;
    float roll;
    bool isValid = false;
};

struct AccelerometerData {
    float aX, aY, aZ;
    bool isValid = false;
};

struct GyroscopeData {
    float gX, gY, gZ;
    bool isValid = false;
};

struct MagnetometerData {
    float mX, mY, mZ;
    bool isValid = false;
};

struct WheelRPMs {
    float leftWheelRPM;
    float rightWheelRPM;
};

struct MuxChannel {
    const char* name;
    bool A0;
    bool A1;
    bool A2;
};

// 2/27/25 TODO: Calibrate against black/white surfaces. See color_sensor_calibration project. Black/white values are used in the matrix method
struct CalibrationValues {
    float redRedValue;
    float greenRedValue;
    float blueRedValue;
    float redGreenValue;
    float greenGreenValue;
    float blueGreenValue;
    float redBlueValue;
    float greenBlueValue;
    float blueBlueValue;
};

struct ColorSensorData {
    uint8_t redValue;
    uint8_t greenValue;
    uint8_t blueValue;
};

struct SideTofCounts {
    uint16_t leftCounts;
    uint16_t rightCounts;
};

namespace LedTypes {
    enum AnimationType {
        NONE,
        BREATHING,
        STROBING,
        RAINBOW
    };
}

namespace Demo {
    enum DemoType {
        NONE,
        BALANCE_CONTROLLER,
        OBSTACLE_AVOIDER
    };
}

struct LedState {
    uint8_t colors[8][3];  // Colors for all 8 LEDs
    LedTypes::AnimationType animation;
    int animationSpeed;
    bool wasAnimationActive;
};

struct BatteryState {
    unsigned int stateOfCharge = 0;      // Battery percentage (0-100%)
    unsigned int voltage = 0;            // Battery voltage (mV)
    int current = 0;                     // Current draw/charge (mA, + = discharging, - = charging)
    int power = 0;                       // Power consumption (mW)
    unsigned int remainingCapacity = 0;  // Remaining capacity (mAh)
    unsigned int fullCapacity = 0;       // Full capacity (mAh)
    int health = 0;                      // Battery health (0-100%)
    bool isCharging = false;             // True if battery is charging
    bool isDischarging = false;          // True if battery is discharging
    // TODO 8/18/25: Remove isLowBattery, isCriticalBattery. We'll do this in the UI
    bool isLowBattery = false;           // True if battery is below threshold
    bool isCriticalBattery = false;      // True if battery is critically low
    float estimatedTimeToEmpty = 0.0;    // Hours until empty (0 if charging/standby)
    float estimatedTimeToFull = 0.0;     // Hours until full (0 if discharging/standby)
    bool isInitialized = false;          // True if BQ27441 is successfully initialized
};

enum class RouteType {
    REGISTER,
    SENSOR_DATA,
    BYTECODE_STATUS,
    WIFI_CONNECTION_RESULT,
    PIP_ID,
    SAVED_NETWORKS,
    SCAN_RESULT_ITEM,
    SCAN_COMPLETE,
    SCAN_STARTED,
    MOTORS_DISABLED_USB,
    PROGRAM_PAUSED_USB,
    BATTERY_MONITOR_DATA_ITEM,
    BATTERY_MONITOR_DATA_COMPLETE,
    BATTERY_MONITOR_DATA_FULL, // This is for sending data over WS
    PIP_TURNING_OFF
};
