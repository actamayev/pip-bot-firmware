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
        AccelerometerData currentAccelData;
        GyroscopeData currentGyroData;
        MagnetometerData currentMagnetometer;

        unsigned long lastReadTime = 0;
        const unsigned long READ_INTERVAL_MILLIS = 20; // 20 millisecond cooldown between reads

        bool shouldUpdate() {
            unsigned long currentMillis = millis();
            // Check if enough time has passed or if this is the first read
            if (currentMillis - lastReadTime >= READ_INTERVAL_MILLIS || lastReadTime == 0) {
                lastReadTime = currentMillis;
                return true;
            }
            return false;
        }

        // New method to update all sensor data at once
        bool updateAllSensorData();
};
