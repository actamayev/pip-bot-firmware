#pragma once
#include <atomic>
#include <math.h>
#include "utils/utils.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "networking/serial_queue_manager.h"

// Combined sensor data structure
struct ImuSample {
    EulerAngles eulerAngles;
    QuaternionData quaternion;
    AccelerometerData accelerometer;
    GyroscopeData gyroscope;
    MagnetometerData magnetometer;
    
    ImuSample() {
        // Initialize all data as invalid with zero values
        eulerAngles.yaw = 0.0f;
        eulerAngles.pitch = 0.0f;
        eulerAngles.roll = 0.0f;
        eulerAngles.isValid = false;
        
        quaternion.qX = 0.0f;
        quaternion.qY = 0.0f;
        quaternion.qZ = 0.0f;
        quaternion.qW = 0.0f;
        quaternion.isValid = false;
        
        accelerometer.aX = 0.0f;
        accelerometer.aY = 0.0f;
        accelerometer.aZ = 0.0f;
        accelerometer.isValid = false;
        
        gyroscope.gX = 0.0f;
        gyroscope.gY = 0.0f;
        gyroscope.gZ = 0.0f;
        gyroscope.isValid = false;
        
        magnetometer.mX = 0.0f;
        magnetometer.mY = 0.0f;
        magnetometer.mZ = 0.0f;
        magnetometer.isValid = false;
    }
};

// Timeout tracking for each report type
struct ReportTimeouts {
    std::atomic<uint32_t> quaternion_last_request{0};
    std::atomic<uint32_t> accelerometer_last_request{0};
    std::atomic<uint32_t> gyroscope_last_request{0};
    std::atomic<uint32_t> magnetometer_last_request{0};
    
    static constexpr uint32_t TIMEOUT_MS = 60000; // 1 minute
    
    bool shouldEnableQuaternion() const {
        return (millis() - quaternion_last_request.load()) < TIMEOUT_MS;
    }
    
    bool shouldEnableAccelerometer() const {
        return (millis() - accelerometer_last_request.load()) < TIMEOUT_MS;
    }
    
    bool shouldEnableGyroscope() const {
        return (millis() - gyroscope_last_request.load()) < TIMEOUT_MS;
    }
    
    bool shouldEnableMagnetometer() const {
        return (millis() - magnetometer_last_request.load()) < TIMEOUT_MS;
    }
};

class SensorDataBuffer : public Singleton<SensorDataBuffer> {
    friend class Singleton<SensorDataBuffer>;

    public:
        // Write methods (called by sensor polling task on Core 0)
        void updateImuSample(const ImuSample& sample);
        void updateQuaternion(const QuaternionData& quaternion);
        void updateEulerAngles(const EulerAngles& euler);
        void updateAccelerometer(const AccelerometerData& accel);
        void updateGyroscope(const GyroscopeData& gyro);
        void updateMagnetometer(const MagnetometerData& mag);
        
        // Read methods (called from any core, resets timeouts)
        EulerAngles getLatestEulerAngles();
        QuaternionData getLatestQuaternion();
        AccelerometerData getLatestAccelerometer();
        GyroscopeData getLatestGyroscope();
        MagnetometerData getLatestMagnetometer();
        
        // Convenience methods for individual values
        float getLatestPitch();
        float getLatestYaw();
        float getLatestRoll();
        float getLatestXAccel();
        float getLatestYAccel();
        float getLatestZAccel();
        float getLatestXRotationRate();
        float getLatestYRotationRate();
        float getLatestZRotationRate();
        double getLatestAccelMagnitude();
        float getLatestMagneticFieldX();
        float getLatestMagneticFieldY();
        float getLatestMagneticFieldZ();

        // Timeout checking (called by IMU sensor to determine what to enable)
        ReportTimeouts& getReportTimeouts() { return timeouts; }
        
        // Helper methods for bulk polling control
        void startPollingAllSensors();
        void stopPollingAllSensors();
        
        // Get complete sample (for debugging/logging)
        ImuSample getLatestImuSample();
        
        // Check if any data is available
        bool hasValidData() const;
        
    private:
        SensorDataBuffer() = default;
        
        // Thread-safe data storage
        ImuSample currentSample;
        std::atomic<uint32_t> lastUpdateTime{0};
        
        // Timeout tracking for each report type
        ReportTimeouts timeouts;
        
        // Helper to update timestamp
        void markDataUpdated();
};
