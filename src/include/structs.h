#pragma once
#include <Arduino.h>
#include <SparkFun_VL53L5CX_Library.h>

struct WiFiCredentials {
	String ssid;
	String password;
};

struct MessageTokens {
    int eventIndex = -1;
    int chunkIndexIndex = -1;
    int totalChunksIndex = -1;
    int totalSizeIndex = -1;
    int isLastIndex = -1;
    int dataIndex = -1;
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
