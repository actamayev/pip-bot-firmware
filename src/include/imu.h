#pragma once
#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "./config.h"
#include "./structs.h"

class ImuSensor {
    public:
        ImuSensor() = default;

        bool initialize();
        bool getData();

        const sh2_SensorValue_t& getSensorValue() const;

        // Quaternion:
        QuaternionData currentQuaternion;
        const QuaternionData& getQuaternion() {
            updateQuaternion();
            return currentQuaternion;
        }

        // Acceleromter:
        AccelerometerData currentAccelData;
        const AccelerometerData& getAccelerometerData() {
            updateAccelerometer();
            return currentAccelData;
        }

        // Gyroscope:
        GyroscopeData currentGyroData;
        const GyroscopeData& getGyroscopeData() {
            updateGyroscope();
            return currentGyroData;
        }

        // Magnetometer:
        MagnetometerData currentMagnetometer;
        const MagnetometerData& getMagnetometerData() {
            updateMagnetometer();
            return currentMagnetometer;
        }
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

        bool updateQuaternion();
        bool updateAccelerometer();
        bool updateGyroscope();
        bool updateMagnetometer();
};
