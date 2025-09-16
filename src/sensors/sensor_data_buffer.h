#pragma once
#include <atomic>
#include <math.h>
#include <vl53l7cx_class.h>
#include "utils/utils.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "networking/serial_queue_manager.h"

// Forward declarations instead of includes
class SerialManager;
class WebSocketManager;  
class BytecodeVM;

// TOF sensor data structure
struct TofData {
    VL53L7CX_ResultsData rawData;
    bool isObjectDetected;
    bool isValid;
    uint32_t timestamp;
    
    TofData() {
        isObjectDetected = false;
        isValid = false;
        timestamp = 0;
        // Initialize rawData to safe defaults
        memset(&rawData, 0, sizeof(VL53L7CX_ResultsData));
    }
};

// Side TOF sensor data structure
struct SideTofData {
    uint16_t leftCounts;
    uint16_t rightCounts;
    bool leftValid;
    bool rightValid;
    uint32_t timestamp;
    
    SideTofData() {
        leftCounts = 0;
        rightCounts = 0;
        leftValid = false;
        rightValid = false;
        timestamp = 0;
    }
};

// Color sensor data structure (using existing ColorSensorData from structs.h)
struct ColorData {
    uint8_t redValue;
    uint8_t greenValue;
    uint8_t blueValue;
    bool isValid;
    uint32_t timestamp;
    
    ColorData() {
        redValue = 0;
        greenValue = 0;
        blueValue = 0;
        isValid = false;
        timestamp = 0;
    }
};

// IR sensor data structure
struct IrData {
    float sensorReadings[5];  // S5, S6, S4, S8, S7
    bool isValid;
    uint32_t timestamp;
    
    IrData() {
        for (int i = 0; i < 5; i++) {
            sensorReadings[i] = 0.0f;
        }
        isValid = false;
        timestamp = 0;
    }
};

// Encoder data structure  
struct EncoderData {
    float leftWheelRPM;
    float rightWheelRPM;
    float distanceTraveledIn;
    int64_t leftEncoderCount;    // Raw encoder count from _leftEncoder.getCount()
    int64_t rightEncoderCount;   // Raw encoder count from _rightEncoder.getCount()
    bool isValid;
    uint32_t timestamp;
    
    EncoderData() {
        leftWheelRPM = 0.0f;
        rightWheelRPM = 0.0f;
        distanceTraveledIn = 0.0f;
        leftEncoderCount = 0;
        rightEncoderCount = 0;
        isValid = false;
        timestamp = 0;
    }
};

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
    std::atomic<uint32_t> tof_last_request{0};
    std::atomic<uint32_t> side_tof_last_request{0};
    std::atomic<uint32_t> color_last_request{0};  // Add color sensor timeout tracking
    std::atomic<uint32_t> ir_last_request{0};  // Add IR sensor timeout tracking

    static constexpr uint32_t TIMEOUT_MS = 5000; // 5 seconds
    
    bool shouldEnableQuaternion() const {
        uint32_t lastRequest = quaternion_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
    
    bool shouldEnableAccelerometer() const {
        uint32_t lastRequest = accelerometer_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
    
    bool shouldEnableGyroscope() const {
        uint32_t lastRequest = gyroscope_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
    
    bool shouldEnableMagnetometer() const {
        uint32_t lastRequest = magnetometer_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
    
    bool shouldEnableTof() const {
        uint32_t lastRequest = tof_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
    
    bool shouldEnableSideTof() const {
        uint32_t lastRequest = side_tof_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
    
    bool shouldEnableColor() const {
        uint32_t lastRequest = color_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }

    bool shouldEnableIr() const {
        uint32_t lastRequest = ir_last_request.load();
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
};

class SensorDataBuffer : public Singleton<SensorDataBuffer> {
    friend class Singleton<SensorDataBuffer>;
    friend class ImuSensor;
    friend class MultizoneTofSensor;
    friend class SideTofManager;
    friend class ColorSensor;  // Add color sensor as friend
    friend class IrSensor;  // Add IR sensor as friend
    friend class EncoderManager;  // Add EncoderManager as friend

    public:        
        // IMU Read methods (called from any core, resets timeouts)
        EulerAngles getLatestEulerAngles();
        QuaternionData getLatestQuaternion();
        AccelerometerData getLatestAccelerometer();
        GyroscopeData getLatestGyroscope();
        MagnetometerData getLatestMagnetometer();
        
        // TOF Read methods (called from any core, resets timeouts)
        TofData getLatestTofData();
        VL53L7CX_ResultsData getLatestTofRawData();
        bool isObjectDetectedTof();
        
        // Side TOF Read methods (called from any core, resets timeouts)
        SideTofData getLatestSideTofData();
        uint16_t getLatestLeftSideTofCounts();
        uint16_t getLatestRightSideTofCounts();
        bool isLeftSideTofValid();
        bool isRightSideTofValid();
        
        // Color sensor Read methods (called from any core, resets timeouts)
        ColorData getLatestColorData();
        uint8_t getLatestRedValue();
        uint8_t getLatestGreenValue();
        uint8_t getLatestBlueValue();
        bool isColorDataValid();
        
        // IR sensor Read methods (called from any core, resets timeouts)
        IrData getLatestIrData();
        float getLatestIrSensorReading(uint8_t index);  // index 0-4
        float* getLatestIrSensorReadings();  // Returns array of all 5
        bool isIrDataValid();
        
        // Encoder Read methods (called from any core, resets timeouts)
        EncoderData getLatestEncoderData();
        WheelRPMs getLatestWheelRPMs();  // Returns legacy WheelRPMs struct for compatibility
        float getLatestLeftWheelRPM();
        float getLatestRightWheelRPM();
        float getLatestDistanceTraveledIn();
        bool isEncoderDataValid();
        
        // Raw encoder count access (for motor driver)
        int64_t getLatestLeftEncoderCount();
        int64_t getLatestRightEncoderCount();
        std::pair<int64_t, int64_t> getLatestEncoderCounts(); // Both counts atomically

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

        // Timeout checking (called by sensors to determine what to enable)
        ReportTimeouts& getReportTimeouts() { return timeouts; }
        
        // Helper methods for bulk polling control
        void stopPollingAllSensors();
        
        // Sensor type enum for selective control
        enum class SensorType {
            QUATERNION,
            ACCELEROMETER, 
            GYROSCOPE,
            MAGNETOMETER,
            MULTIZONE_TOF,
            SIDE_TOF,
            COLOR,
            IR
        };
        
        // Selective sensor polling control
        void stopPollingSensor(SensorType sensorType);
        
        // Get complete samples (for debugging/logging)
        ImuSample getLatestImuSample();
        
        // Frequency tracking methods
        float getImuFrequency();
        float getMultizoneTofFrequency();
        float getSideTofFrequency();
        float getColorSensorFrequency();

        bool shouldEnableQuaternionExtended() const;

    private:
        SensorDataBuffer() = default;
        
        // Write methods (called by sensor polling task on Core 0)
        void updateQuaternion(const QuaternionData& quaternion);
        void updateAccelerometer(const AccelerometerData& accel);
        void updateGyroscope(const GyroscopeData& gyro);
        void updateMagnetometer(const MagnetometerData& mag);
        void updateTofData(const TofData& tof);
        void updateSideTofData(const SideTofData& sideTof);
        void updateColorData(const ColorData& color);  // Add color sensor update method
        void updateIrData(const IrData& ir);  // Add IR sensor update method
        void updateEncoderData(const EncoderData& encoder);  // Add encoder update method

        // Thread-safe data storage
        ImuSample currentSample;
        TofData currentTofData;
        SideTofData currentSideTofData;
        ColorData currentColorData;  // Add color sensor data storage
        IrData currentIrData;  // Add IR sensor data storage
        EncoderData currentEncoderData;  // Add encoder data storage

        std::atomic<uint32_t> lastImuUpdateTime{0};
        std::atomic<uint32_t> lastTofUpdateTime{0};
        std::atomic<uint32_t> lastSideTofUpdateTime{0};
        std::atomic<uint32_t> lastColorUpdateTime{0};  // Separate timestamp for color sensor
        std::atomic<uint32_t> lastIrUpdateTime{0};  // Separate timestamp for IR
        std::atomic<uint32_t> lastEncoderUpdateTime{0};  // Separate timestamp for encoders

        // Timeout tracking for each report type
        ReportTimeouts timeouts;
        
        // Frequency tracking for sensors
        std::atomic<uint32_t> imuUpdateCount{0};
        std::atomic<uint32_t> lastImuFrequencyCalcTime{0};
        std::atomic<uint32_t> multizoneTofUpdateCount{0};
        std::atomic<uint32_t> lastMultizoneTofFrequencyCalcTime{0};
        std::atomic<uint32_t> sideTofUpdateCount{0};
        std::atomic<uint32_t> lastSideTofFrequencyCalcTime{0};
        std::atomic<uint32_t> colorSensorUpdateCount{0};
        std::atomic<uint32_t> lastColorSensorFrequencyCalcTime{0};

        // Helper to update timestamp
        void markImuDataUpdated();
        void markTofDataUpdated();
        void markSideTofDataUpdated();
        void markColorDataUpdated();  // Separate method for color sensor timestamp
        void markIrDataUpdated();  // Separate method for IR timestamp
        void markEncoderDataUpdated();  // Separate method for encoder timestamp
};
