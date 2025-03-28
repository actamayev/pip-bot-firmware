#pragma once
#include <Adafruit_BNO08x.h>
#include "../utils/config.h"
#include "../utils/utils.h"
#include "../utils/structs.h"

class ImuSensor {
    public:
        ImuSensor() = default;

        bool initialize();

        // Quaternion:
        const EulerAngles& getEulerAngles();
        float getPitch();
        float getYaw();
        float getRoll();

        // Acceleromter:
        float getXAccel();
        float getYAccel();
        float getZAccel();
        double getAccelMagnitude();
        const AccelerometerData& getAccelerometerData();

        // Gyroscope:
        float getXRotationRate();
        float getYRotationRate();
        float getZRotationRate();
        const GyroscopeData& getGyroscopeData();

        // Magnetometer:
        float getMagneticFieldX();
        float getMagneticFieldY();
        float getMagneticFieldZ();
        const MagnetometerData& getMagnetometerData();

    private:
        Adafruit_BNO08x imu;
        sh2_SensorValue_t sensorValue;
        bool isInitialized = false;
        
        EulerAngles currentEulerAngles = {0, 0, 0, false};
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

        GyroscopeData currentGyroData;
        bool updateGyroscope();

        MagnetometerData currentMagnetometer;
        bool updateMagnetometer();
};
