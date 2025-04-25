#pragma once
#include <Arduino.h>
#include <SparkFun_VL53L5CX_Library.h>

struct WiFiCredentials {
	String ssid;
	String password;
};

struct WiFiNetworkInfo {
    String ssid;
    int32_t rssi;
    uint8_t encryptionType;
};

struct TofData {
    VL53L5CX_ResultsData data;
    bool isValid = false;
    unsigned long lastUpdateTime = 0;
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
    bool isValid;
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
    bool A;
    bool B;
    bool C;
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

struct SideTofDistances {
    uint16_t leftDistance;
    uint16_t rightDistance;
};

namespace LedTypes {
    enum AnimationType {
        NONE,
        BREATHING,
        STROBING,
        RAINBOW,
    };
}
