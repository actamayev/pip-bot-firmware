#pragma once
#include <Adafruit_BNO08x.h>
#include "utils/config.h"
#include "utils/utils.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "sensor_polling_manager.h"

class ImuSensor : public Singleton<ImuSensor> {
    friend class Singleton<ImuSensor>;
    friend class SensorPollingManager;

    public:
        ImuSensor() = default;

        bool initialize();

        // Quaternion:
        const EulerAngles& getEulerAngles();
        float getPitch();
        float getYaw();
        float getRoll();

        // Accelerometer:
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

        bool needsInitialization() const { return !isInitialized; }
        bool canRetryInitialization() const;

        uint8_t getInitRetryCount() const { return initRetryCount; }
        uint8_t getMaxInitRetries() const { return MAX_INIT_RETRIES; }
        void turnOff();

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

        // New method to update all sensor data at once
        bool updateAllSensorData();

        uint8_t initRetryCount = 0;
        const uint8_t MAX_INIT_RETRIES = 3;
        unsigned long lastInitAttempt = 0;
        const unsigned long INIT_RETRY_INTERVAL = 1000; // 1 second between retry attempts
};
