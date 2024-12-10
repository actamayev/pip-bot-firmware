#pragma once
#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "./config.h"

class ImuSensor {
    public:
        ImuSensor() = default;

        bool initialize();

        bool enableGameRotationVector(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);
        bool enableAccelerometer(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);
        bool enableGyroscope(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);
        bool enableMagneticField(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);

        bool getData();

        const sh2_SensorValue_t& getSensorValue() const;

        // Convenience methods to get specific data types
        bool getQuaternion(float& qX, float& qY, float& qZ, float& qW);
        bool getAcceleration(float& aX, float& aY, float& aZ);
        bool getGyroscope(float& gX, float& gY, float& gZ);
        bool getMagneticField(float& mX, float& mY, float& mZ);

    private:
        Adafruit_BNO08x imu;
        sh2_SensorValue_t sensorValue;
        bool isInitialized = false;

        // Store enabled reports for status checking
        struct EnabledReports {
            bool gameRotationVector = false;
            bool accelerometer = false;
            bool gyroscope = false;
            bool magneticField = false;
        } enabledReports;
};
