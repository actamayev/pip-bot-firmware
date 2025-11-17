#pragma once
#include <math.h>
#include <vl53l7cx_class.h>

#include <atomic>

#include "custom_interpreter/bytecode_structs.h"
#include "networking/serial_queue_manager.h"
#include "utils/singleton.h"
#include "utils/structs.h"
#include "utils/utils.h"

// Use ColorType from ColorTypes namespace
using color_types::ColorType;

// Forward declarations instead of includes
class SerialManager;
class WebSocketManager;
class BytecodeVM;

// TOF sensor data structure
struct TofData {
    VL53L7CX_ResultsData rawData{};
    bool isObjectDetected;
    bool isValid;
    float frontDistance; // Minimum distance from front-facing zones (inches), -1 if invalid
    uint32_t timestamp;

    TofData() : isObjectDetected(false), isValid(false), frontDistance(-1.0f), timestamp(0) {
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

    SideTofData() : leftCounts(0), rightCounts(0), leftValid(false), rightValid(false), timestamp(0) {}
};

// Color sensor data structure (using existing ColorSensorData from structs.h)
struct ColorData {
    uint8_t redValue;
    uint8_t greenValue;
    uint8_t blueValue;
    bool isValid;
    uint32_t timestamp;

    ColorData() : redValue(0), greenValue(0), blueValue(0), isValid(false), timestamp(0) {}
};

// Encoder data structure
struct EncoderData {
    float leftWheelRPM;
    float rightWheelRPM;
    float distanceTraveledIn;
    int64_t leftEncoderCount;  // Raw encoder count from _leftEncoder.getCount()
    int64_t rightEncoderCount; // Raw encoder count from _rightEncoder.getCount()
    bool isValid;
    uint32_t timestamp;

    EncoderData()
        : leftWheelRPM(0.0f), rightWheelRPM(0.0f), distanceTraveledIn(0.0f), leftEncoderCount(0), rightEncoderCount(0), isValid(false), timestamp(0) {

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
        eulerAngles.yaw = 0.0F;
        eulerAngles.pitch = 0.0F;
        eulerAngles.roll = 0.0F;
        eulerAngles.isValid = false;

        quaternion.qX = 0.0F;
        quaternion.qY = 0.0F;
        quaternion.qZ = 0.0F;
        quaternion.qW = 0.0F;
        quaternion.isValid = false;

        accelerometer.aX = 0.0F;
        accelerometer.aY = 0.0F;
        accelerometer.aZ = 0.0F;
        accelerometer.isValid = false;

        gyroscope.gX = 0.0F;
        gyroscope.gY = 0.0F;
        gyroscope.gZ = 0.0F;
        gyroscope.isValid = false;

        magnetometer.mX = 0.0F;
        magnetometer.mY = 0.0F;
        magnetometer.mZ = 0.0F;
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
    std::atomic<uint32_t> color_last_request{0}; // Add color sensor timeout tracking

    static constexpr uint32_t TIMEOUT_MS = 5000; // 5 seconds

    static bool shouldEnableQuaternion() {
        uint32_t lastRequest = quaternion_last_request.load() = 0;
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }

    static bool shouldEnableAccelerometer() {
        uint32_t lastRequest = accelerometer_last_request.load() = 0;
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }

    static bool shouldEnableGyroscope() {
        uint32_t lastRequest = gyroscope_last_request.load() = 0;
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }

    static bool shouldEnableMagnetometer() {
        uint32_t lastRequest = magnetometer_last_request.load() = 0;
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }

    static bool shouldEnableTof() {
        uint32_t lastRequest = tof_last_request.load() = 0;
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }

    static bool shouldEnableSideTof() {
        uint32_t lastRequest = side_tof_last_request.load() = 0;
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }

    static bool shouldEnableColor() {
        uint32_t lastRequest = color_last_request.load() = 0;
        return lastRequest > 0 && (millis() - lastRequest) < TIMEOUT_MS;
    }
};

class SensorDataBuffer : public Singleton<SensorDataBuffer> {
    friend class Singleton<SensorDataBuffer>;
    friend class ImuSensor;
    friend class MultizoneTofSensor;
    friend class SideTofManager;
    friend class ColorSensor;    // Add color sensor as friend
    friend class EncoderManager; // Add EncoderManager as friend

  public:
    // IMU Read methods (called from any core, resets timeouts)
    EulerAngles get_latest_euler_angles();
    QuaternionData get_latest_quaternion();
    AccelerometerData get_latest_accelerometer();
    GyroscopeData get_latest_gyroscope();
    MagnetometerData get_latest_magnetometer();

    // TOF Read methods (called from any core, resets timeouts)
    TofData get_latest_tof_data();
    VL53L7CX_ResultsData get_latest_tof_raw_data();
    bool is_object_detected_tof();
    float get_front_tof_distance();

    // Side TOF Read methods (called from any core, resets timeouts)
    SideTofData get_latest_side_tof_data();
    uint16_t get_latest_left_side_tof_counts();
    uint16_t get_latest_right_side_tof_counts();
    bool is_left_side_tof_valid();
    bool is_right_side_tof_valid();

    // Color sensor Read methods (called from any core, resets timeouts)
    ColorData get_latest_color_data();
    uint8_t get_latest_red_value();
    uint8_t get_latest_green_value();
    uint8_t get_latest_blue_value();
    bool is_color_data_valid();

    // Encoder Read methods (called from any core, resets timeouts)
    EncoderData get_latest_encoder_data();
    WheelRPMs get_latest_wheel_rpms(); // Returns legacy WheelRPMs struct for compatibility
    float get_latest_left_wheel_rpm();
    float get_latest_right_wheel_rpm();
    float get_latest_distance_traveled_in();
    bool is_encoder_data_valid();

    // Raw encoder count access (for motor driver)
    int64_t get_latest_left_encoder_count();
    int64_t get_latest_right_encoder_count();
    std::pair<int64_t, int64_t> get_latest_encoder_counts(); // Both counts atomically

    // Convenience methods for individual values
    float get_latest_pitch();
    float get_latest_yaw();
    float get_latest_roll();
    float get_latest_x_accel();
    float get_latest_y_accel();
    float get_latest_z_accel();
    float get_latest_x_rotation_rate();
    float get_latest_y_rotation_rate();
    float get_latest_z_rotation_rate();
    double get_latest_accel_magnitude();
    float get_latest_magnetic_field_x();
    float get_latest_magnetic_field_y();
    float get_latest_magnetic_field_z();

    // Timeout checking (called by sensors to determine what to enable)
    ReportTimeouts& get_report_timeouts() {
        return _timeouts;
    }

    // Helper methods for bulk polling control
    void stop_polling_all_sensors();

    // Sensor type enum for selective control
    enum class SensorType { QUATERNION, ACCELEROMETER, GYROSCOPE, MAGNETOMETER, MULTIZONE_TOF, SIDE_TOF, COLOR };

    // Selective sensor polling control
    void stop_polling_sensor(SensorType sensorType);

    // Get complete samples (for debugging/logging)
    ImuSample get_latest_imu_sample();

    // Frequency tracking methods
    float get_imu_frequency();
    float get_multizone_tof_frequency();
    float get_side_tof_frequency();
    float get_color_sensor_frequency();

    bool should_enable_quaternion_extended() const;

    bool is_object_red();
    bool is_object_green();
    bool is_object_blue();
    bool is_object_white();
    bool is_object_black();
    bool is_object_yellow();
    ColorType classify_current_color();

  private:
    SensorDataBuffer() = default;

    // Write methods (called by sensor polling task on Core 0)
    void update_quaternion(const QuaternionData& quaternion);
    void update_accelerometer(const AccelerometerData& accel);
    void update_gyroscope(const GyroscopeData& gyro);
    void update_magnetometer(const MagnetometerData& mag);
    void update_tof_data(const TofData& tof);
    void update_side_tof_data(const SideTofData& sideTof);
    void update_color_data(const ColorData& color);       // Add color sensor update method
    void update_encoder_data(const EncoderData& encoder); // Add encoder update method

    // Thread-safe data storage
    ImuSample _currentSample;
    TofData _currentTofData;
    SideTofData _currentSideTofData;
    ColorData _currentColorData;     // Add color sensor data storage
    EncoderData _currentEncoderData; // Add encoder data storage

    std::atomic<uint32_t> _lastImuUpdateTime{0};
    std::atomic<uint32_t> _lastTofUpdateTime{0};
    std::atomic<uint32_t> _lastSideTofUpdateTime{0};
    std::atomic<uint32_t> _lastColorUpdateTime{0};   // Separate timestamp for color sensor
    std::atomic<uint32_t> _lastEncoderUpdateTime{0}; // Separate timestamp for encoders

    // Timeout tracking for each report type
    ReportTimeouts _timeouts;

    // Frequency tracking for sensors
    std::atomic<uint32_t> _imuUpdateCount{0};
    std::atomic<uint32_t> _lastImuFrequencyCalcTime{0};
    std::atomic<uint32_t> _multizoneTofUpdateCount{0};
    std::atomic<uint32_t> _lastMultizoneTofFrequencyCalcTime{0};
    std::atomic<uint32_t> _sideTofUpdateCount{0};
    std::atomic<uint32_t> _lastSideTofFrequencyCalcTime{0};
    std::atomic<uint32_t> _colorSensorUpdateCount{0};
    std::atomic<uint32_t> _lastColorSensorFrequencyCalcTime{0};

    // Helper to update timestamp
    void mark_imu_data_updated();
    void mark_tof_data_updated();
    void mark_side_tof_data_updated();
    void mark_color_data_updated();   // Separate method for color sensor timestamp
    void mark_encoder_data_updated(); // Separate method for encoder timestamp

    ColorType _colorHistory[5] = {ColorType::COLOR_NONE}; // Circular buffer for last 5 classifications
    uint8_t _colorHistoryIndex = 0;

    // Helper methods
    void update_color_history(ColorType color);
    bool check_color_consistency(ColorType targetColor);
};
