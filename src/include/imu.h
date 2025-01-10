#pragma once
#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "./utils.h"
#include "./config.h"
#include "./structs.h"

class ImuSensor {
    public:
        ImuSensor() = default;

        bool initialize();

        // Quaternion:
        EulerAngles& getEulerAngles();
        float getPitch();
        float getYaw();
        float getRoll();

        // Acceleromter:
        float getXAccel();
        float getYAccel();
        float getZAccel();
        double getAccelMagnitude();

        // Gyroscope:
        float getXRotationRate();
        float getYRotationRate();
        float getZRotationRate();

        // Magnetometer:
        float getMagneticFieldX();
        float getMagneticFieldY();
        float getMagneticFieldZ();

    private:
        Adafruit_BNO08x imu;
        sh2_SensorValue_t sensorValue;
        bool isInitialized = false;

        bool enableGameRotationVector();
        bool enableAccelerometer();
        bool enableGyroscope();
        bool enableMagneticField();

        // Store enabled reports for status checking
        EnabledReports enabledReports;

        bool getImuData();

        QuaternionData currentQuaternion;
        bool updateQuaternion();
        const QuaternionData& getQuaternion();

        AccelerometerData currentAccelData;
        bool updateAccelerometer();
        const AccelerometerData& getAccelerometerData();

        GyroscopeData currentGyroData;
        bool updateGyroscope();
        const GyroscopeData& getGyroscopeData();

        MagnetometerData currentMagnetometer;
        bool updateMagnetometer();
        const MagnetometerData& getMagnetometerData();
};
