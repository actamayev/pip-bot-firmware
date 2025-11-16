#pragma once
#include <Adafruit_BNO08x.h>

#include "sensor_data_buffer.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "utils/structs.h"
#include "utils/utils.h"

class ImuSensor : public Singleton<ImuSensor> {
    friend class Singleton<ImuSensor>;
    friend class TaskManager;
    friend class SensorInitializer;

  private:
    ImuSensor() = default;
    bool initialize();
    void turnOff();
    Adafruit_BNO08x imu;
    sh2_SensorValue_t sensorValue;
    bool isInitialized = false;

    // Report management based on timeout system
    EnabledReports enabledReports;
    void updateEnabledReports(); // Check timeouts and enable/disable reports
    void enableGameRotationVector();
    void enableAccelerometer();
    void enableGyroscope();
    void enableMagneticField();

    void disableGameRotationVector();
    void disableAccelerometer();
    void disableGyroscope();
    void disableMagneticField();

    const uint16_t IMU_UPDATE_FREQ_MICROSECS = 5000; // 5ms, 200Hz
    const uint8_t IMU_DEFAULT_ADDRESS = 0x4A;        // 5ms, 200Hz

    // Polling control
    void updateSensorData(); // Single read, write to buffer
    bool shouldBePolling() const;
};
