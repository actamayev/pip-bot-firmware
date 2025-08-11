#pragma once
#include <Adafruit_BNO08x.h>
#include "utils/utils.h"
#include "utils/config.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "sensor_data_buffer.h"

class ImuSensor : public Singleton<ImuSensor> {
    friend class Singleton<ImuSensor>;

    public:
        ImuSensor() = default;

        bool initialize();

        // Polling control
        void updateSensorData();  // Single read, write to buffer
        bool shouldBePolling() const;

        bool needsInitialization() const { return !isInitialized; }
        bool canRetryInitialization() const;

        uint8_t getInitRetryCount() const { return initRetryCount; }
        uint8_t getMaxInitRetries() const { return MAX_INIT_RETRIES; }
        void turnOff();

    private:
        Adafruit_BNO08x imu;
        sh2_SensorValue_t sensorValue;
        bool isInitialized = false;

        // Report management based on timeout system
        EnabledReports enabledReports;
        void updateEnabledReports();  // Check timeouts and enable/disable reports

        void enableGameRotationVector();
        void enableAccelerometer();
        void enableGyroscope();
        void enableMagneticField();

        void disableGameRotationVector();
        void disableAccelerometer();
        void disableGyroscope();
        void disableMagneticField();

        uint8_t initRetryCount = 0;
        const uint8_t MAX_INIT_RETRIES = 3;
        unsigned long lastInitAttempt = 0;
        const unsigned long INIT_RETRY_INTERVAL = 1000; // 1 second between retry attempts
};
