#pragma once
#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "./config.h"
#include "./structs.h"
#include "./utils.h"

class ImuSensor {
    public:
        ImuSensor() = default;

        bool initialize();

        // Quaternion:
        QuaternionData currentQuaternion;
        EulerAngles& getEulerAngles();
        float getPitch();
        float getYaw();
        float getRoll();

        // Acceleromter:
        AccelerometerData currentAccelData;
        float getXAccel();
        float getYAccel();
        float getZAccel();

        // Gyroscope:
        GyroscopeData currentGyroData;
        float getXRotationRate();
        float getYRotationRate();
        float getZRotationRate();

        // Magnetometer:
        MagnetometerData currentMagnetometer;
        float getMagneticFieldX();
        float getMagneticFieldY();
        float getMagneticFieldZ();

    private:
        Adafruit_BNO08x imu;
        sh2_SensorValue_t sensorValue;
        bool isInitialized = false;

        bool enableGameRotationVector(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);
        bool enableAccelerometer(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);
        bool enableGyroscope(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);
        bool enableMagneticField(uint32_t updateFreqMicros = IMU_UPDATE_FREQ_MICROSECS);

        // Store enabled reports for status checking
        struct EnabledReports {
            bool gameRotationVector = false;
            bool accelerometer = false;
            bool gyroscope = false;
            bool magneticField = false;
        } enabledReports;

        bool getData();

        bool updateQuaternion();
        const QuaternionData& getQuaternion();

        bool updateAccelerometer();
        const AccelerometerData& getAccelerometerData();

        bool updateGyroscope();
        const GyroscopeData& getGyroscopeData();

        bool updateMagnetometer();
        const MagnetometerData& getMagnetometerData();
};
