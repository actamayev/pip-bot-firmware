#include "sensor_data_buffer.h"

#include "custom_interpreter/bytecode_vm.h"
#include "networking/serial_manager.h"
#include "networking/websocket_manager.h"

// Use ColorType from ColorTypes namespace
using ColorTypes::ColorType;

// IMU update methods (existing)
void SensorDataBuffer::update_quaternion(const QuaternionData& quaternion) {
    currentSample.quaternion = quaternion;
    // Update derived Euler angles if quaternion is valid
    if (quaternion.isValid) {
        quaternionToEuler(quaternion.qW, quaternion.qX, quaternion.qY, quaternion.qZ, currentSample.eulerAngles.yaw, currentSample.eulerAngles.pitch,
                          currentSample.eulerAngles.roll);
        currentSample.eulerAngles.isValid = true;
    }
    mark_imu_data_updated();
}

void SensorDataBuffer::update_accelerometer(const AccelerometerData& accel) {
    currentSample.accelerometer = accel;
    mark_imu_data_updated();
}

void SensorDataBuffer::update_gyroscope(const GyroscopeData& gyro) {
    currentSample.gyroscope = gyro;
    mark_imu_data_updated();
}

void SensorDataBuffer::update_magnetometer(const MagnetometerData& mag) {
    currentSample.magnetometer = mag;
    mark_imu_data_updated();
}

// TOF update method (existing)
void SensorDataBuffer::update_tof_data(const TofData& tof) {
    currentTofData = tof;
    mark_tof_data_updated();
}

// Side TOF update method (existing)
void SensorDataBuffer::update_side_tof_data(const SideTofData& sideTof) {
    currentSideTofData = sideTof;
    mark_side_tof_data_updated();
}

// NEW: Color sensor update method
void SensorDataBuffer::update_color_data(const ColorData& color) {
    currentColorData = color;
    mark_color_data_updated();
}

// NEW: Encoder update method
void SensorDataBuffer::update_encoder_data(const EncoderData& encoder) {
    currentEncoderData = encoder;
    mark_encoder_data_updated();
}

// IMU Read methods - reset timeouts when called (existing)
EulerAngles SensorDataBuffer::get_latest_euler_angles() {
    timeouts.quaternion_last_request.store(millis());
    return currentSample.eulerAngles;
}

QuaternionData SensorDataBuffer::get_latest_quaternion() {
    timeouts.quaternion_last_request.store(millis());
    return currentSample.quaternion;
}

AccelerometerData SensorDataBuffer::get_latest_accelerometer() {
    timeouts.accelerometer_last_request.store(millis());
    return currentSample.accelerometer;
}

GyroscopeData SensorDataBuffer::get_latest_gyroscope() {
    timeouts.gyroscope_last_request.store(millis());
    return currentSample.gyroscope;
}

MagnetometerData SensorDataBuffer::get_latest_magnetometer() {
    timeouts.magnetometer_last_request.store(millis());
    return currentSample.magnetometer;
}

// TOF Read methods - reset timeouts when called (existing)
TofData SensorDataBuffer::get_latest_tof_data() {
    timeouts.tof_last_request.store(millis());
    return currentTofData;
}

VL53L7CX_ResultsData SensorDataBuffer::get_latest_tof_raw_data() {
    timeouts.tof_last_request.store(millis());
    return currentTofData.rawData;
}

bool SensorDataBuffer::is_object_detected_tof() {
    timeouts.tof_last_request.store(millis());
    return currentTofData.isObjectDetected && currentTofData.isValid;
}

float SensorDataBuffer::get_front_tof_distance() {
    timeouts.tof_last_request.store(millis());
    return currentTofData.frontDistance;
}

// Side TOF Read methods - reset timeouts when called (existing)
SideTofData SensorDataBuffer::get_latest_side_tof_data() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData;
}

uint16_t SensorDataBuffer::get_latest_left_side_tof_counts() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.leftCounts;
}

uint16_t SensorDataBuffer::get_latest_right_side_tof_counts() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.rightCounts;
}

bool SensorDataBuffer::is_left_side_tof_valid() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.leftValid;
}

bool SensorDataBuffer::is_right_side_tof_valid() {
    timeouts.side_tof_last_request.store(millis());
    return currentSideTofData.rightValid;
}

// NEW: Color sensor Read methods - reset timeouts when called
ColorData SensorDataBuffer::get_latest_color_data() {
    timeouts.color_last_request.store(millis());
    return currentColorData;
}

uint8_t SensorDataBuffer::get_latest_red_value() {
    timeouts.color_last_request.store(millis());
    return currentColorData.redValue;
}

uint8_t SensorDataBuffer::get_latest_green_value() {
    timeouts.color_last_request.store(millis());
    return currentColorData.greenValue;
}

uint8_t SensorDataBuffer::get_latest_blue_value() {
    timeouts.color_last_request.store(millis());
    return currentColorData.blueValue;
}

bool SensorDataBuffer::is_color_data_valid() {
    timeouts.color_last_request.store(millis());
    return currentColorData.isValid;
}

// NEW: Encoder Read methods - reset timeouts when called
EncoderData SensorDataBuffer::get_latest_encoder_data() {
    return currentEncoderData;
}

WheelRPMs SensorDataBuffer::get_latest_wheel_rpms() {
    WheelRPMs rpms;
    rpms.leftWheelRPM = currentEncoderData.leftWheelRPM;
    rpms.rightWheelRPM = currentEncoderData.rightWheelRPM;
    return rpms;
}

float SensorDataBuffer::get_latest_left_wheel_rpm() {
    return currentEncoderData.leftWheelRPM;
}

float SensorDataBuffer::get_latest_right_wheel_rpm() {
    return currentEncoderData.rightWheelRPM;
}

float SensorDataBuffer::get_latest_distance_traveled_in() {
    return currentEncoderData.distanceTraveledIn;
}

bool SensorDataBuffer::is_encoder_data_valid() {
    return currentEncoderData.isValid;
}

// Raw encoder count access methods (for motor driver)
int64_t SensorDataBuffer::get_latest_left_encoder_count() {
    return currentEncoderData.leftEncoderCount;
}

int64_t SensorDataBuffer::get_latest_right_encoder_count() {
    return currentEncoderData.rightEncoderCount;
}

std::pair<int64_t, int64_t> SensorDataBuffer::get_latest_encoder_counts() {
    return std::make_pair(currentEncoderData.leftEncoderCount, currentEncoderData.rightEncoderCount);
}

// Convenience methods for individual values (existing)
float SensorDataBuffer::get_latest_pitch() {
    return get_latest_euler_angles().roll; // Note: roll maps to pitch in your system
}

float SensorDataBuffer::get_latest_yaw() {
    return get_latest_euler_angles().yaw;
}

float SensorDataBuffer::get_latest_roll() {
    return get_latest_euler_angles().pitch; // Note: pitch maps to roll in your system
}

float SensorDataBuffer::get_latest_x_accel() {
    return get_latest_accelerometer().aX;
}

float SensorDataBuffer::get_latest_y_accel() {
    return get_latest_accelerometer().aY;
}

float SensorDataBuffer::get_latest_z_accel() {
    return get_latest_accelerometer().aZ;
}

float SensorDataBuffer::get_latest_x_rotation_rate() {
    return get_latest_gyroscope().gX;
}

float SensorDataBuffer::get_latest_y_rotation_rate() {
    return get_latest_gyroscope().gY;
}

float SensorDataBuffer::get_latest_z_rotation_rate() {
    return get_latest_gyroscope().gZ;
}

double SensorDataBuffer::get_latest_accel_magnitude() {
    return sqrt(pow(get_latest_x_accel(), 2) + pow(get_latest_y_accel(), 2) + pow(get_latest_z_accel(), 2));
}

float SensorDataBuffer::get_latest_magnetic_field_x() {
    return get_latest_magnetometer().mX;
}

float SensorDataBuffer::get_latest_magnetic_field_y() {
    return get_latest_magnetometer().mY;
}

float SensorDataBuffer::get_latest_magnetic_field_z() {
    return get_latest_magnetometer().mZ;
}

ImuSample SensorDataBuffer::get_latest_imu_sample() {
    // Mark all timeouts as accessed
    uint32_t currentTime = millis();
    timeouts.quaternion_last_request.store(currentTime);
    timeouts.accelerometer_last_request.store(currentTime);
    timeouts.gyroscope_last_request.store(currentTime);
    timeouts.magnetometer_last_request.store(currentTime);

    return currentSample;
}

void SensorDataBuffer::stop_polling_all_sensors() {
    stop_polling_sensor(SensorType::QUATERNION);
    stop_polling_sensor(SensorType::ACCELEROMETER);
    stop_polling_sensor(SensorType::GYROSCOPE);
    stop_polling_sensor(SensorType::MAGNETOMETER);
    stop_polling_sensor(SensorType::MULTIZONE_TOF);
    stop_polling_sensor(SensorType::SIDE_TOF);
    stop_polling_sensor(SensorType::COLOR);
}

void SensorDataBuffer::stop_polling_sensor(SensorType sensorType) {
    switch (sensorType) {
        case SensorType::QUATERNION:
            timeouts.quaternion_last_request.store(0);
            break;
        case SensorType::ACCELEROMETER:
            timeouts.accelerometer_last_request.store(0);
            break;
        case SensorType::GYROSCOPE:
            timeouts.gyroscope_last_request.store(0);
            break;
        case SensorType::MAGNETOMETER:
            timeouts.magnetometer_last_request.store(0);
            break;
        case SensorType::MULTIZONE_TOF:
            timeouts.tof_last_request.store(0);
            break;
        case SensorType::SIDE_TOF:
            timeouts.side_tof_last_request.store(0);
            break;
        case SensorType::COLOR:
            timeouts.color_last_request.store(0);
            break;
    }
}

void SensorDataBuffer::mark_imu_data_updated() {
    lastImuUpdateTime.store(millis());
    imuUpdateCount.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_tof_data_updated() {
    lastTofUpdateTime.store(millis());
    multizoneTofUpdateCount.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_side_tof_data_updated() {
    lastSideTofUpdateTime.store(millis());
    sideTofUpdateCount.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_color_data_updated() {
    lastColorUpdateTime.store(millis());
    colorSensorUpdateCount.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_encoder_data_updated() {
    lastEncoderUpdateTime.store(millis());
}

float SensorDataBuffer::get_imu_frequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;

    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastImuFrequencyCalcTime.load();
    uint32_t currentUpdateCount = imuUpdateCount.load();

    // Initialize on first call
    if (lastCalcTime == 0) {
        lastImuFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }

    uint32_t timeDelta = currentTime - lastCalcTime;

    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;

        // Debug logging
        char debugBuffer[128];
        snprintf(debugBuffer, sizeof(debugBuffer), "DEBUG: updateDelta=%u, timeDelta=%u, freq=%.1f", updateDelta, timeDelta, lastFrequency);
        SerialQueueManager::get_instance().queue_message(debugBuffer);

        // Update tracking variables
        lastImuFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;

        return lastFrequency;
    }

    // Return last calculated frequency if not time to update yet
    return lastFrequency;
}

float SensorDataBuffer::get_multizone_tof_frequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;

    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastMultizoneTofFrequencyCalcTime.load();
    uint32_t currentUpdateCount = multizoneTofUpdateCount.load();

    // Initialize on first call
    if (lastCalcTime == 0) {
        lastMultizoneTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }

    uint32_t timeDelta = currentTime - lastCalcTime;

    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;

        // Update tracking variables
        lastMultizoneTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;

        return lastFrequency;
    }

    return lastFrequency;
}

float SensorDataBuffer::get_side_tof_frequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;

    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastSideTofFrequencyCalcTime.load();
    uint32_t currentUpdateCount = sideTofUpdateCount.load();

    // Initialize on first call
    if (lastCalcTime == 0) {
        lastSideTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }

    uint32_t timeDelta = currentTime - lastCalcTime;

    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;

        // Update tracking variables
        lastSideTofFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;

        return lastFrequency;
    }

    return lastFrequency;
}

float SensorDataBuffer::get_color_sensor_frequency() {
    static float lastFrequency = 0.0f;
    static uint32_t lastUpdateCount = 0;

    uint32_t currentTime = millis();
    uint32_t lastCalcTime = lastColorSensorFrequencyCalcTime.load();
    uint32_t currentUpdateCount = colorSensorUpdateCount.load();

    // Initialize on first call
    if (lastCalcTime == 0) {
        lastColorSensorFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;
        return 0.0f;
    }

    uint32_t timeDelta = currentTime - lastCalcTime;

    // Calculate frequency every second (1000ms)
    if (timeDelta >= 1000) {
        uint32_t updateDelta = currentUpdateCount - lastUpdateCount;
        lastFrequency = (float)updateDelta * 1000.0f / (float)timeDelta;

        // Update tracking variables
        lastColorSensorFrequencyCalcTime.store(currentTime);
        lastUpdateCount = currentUpdateCount;

        return lastFrequency;
    }

    return lastFrequency;
}

bool SensorDataBuffer::should_enable_quaternion_extended() const {
    // Check if within timeout window (original condition)
    bool withinTimeout = timeouts.shouldEnableQuaternion();

    // Check if serial is connected
    bool serialConnected = SerialManager::get_instance().is_serial_connected();

    // Check if bytecode program is loaded (including paused)
    bool programLoaded = BytecodeVM::get_instance().is_program_loaded();

    // Check if user is connected via websocket
    bool userConnected = WebSocketManager::get_instance().isUserConnectedToThisPip();

    return withinTimeout || serialConnected || programLoaded || userConnected;
}

// Add these new methods

ColorType SensorDataBuffer::classify_current_color() {
    uint8_t r = currentColorData.redValue;
    uint8_t g = currentColorData.greenValue;
    uint8_t b = currentColorData.blueValue;

    if (!currentColorData.isValid) return ColorType::COLOR_NONE;

    // White: All components bright
    if (r > 130 && g > 130 && b > 130) {
        return ColorType::COLOR_WHITE;
    }

    // Yellow: Red and Green strong, Blue weak
    if (r > 120 && g > 120 && b < 100) {
        return ColorType::COLOR_YELLOW;
    }

    // Red: R dominant and bright enough
    if (r > 80 && r > (g + 30) && r > (b + 30)) {
        return ColorType::COLOR_RED;
    }

    // Green: G dominant and bright enough
    if (g > 65 && g > (r + 20) && g > (b + 20)) {
        return ColorType::COLOR_GREEN;
    }

    // Blue: B dominant and bright enough
    if (b > 65 && b > (r + 20) && b > (g + 20)) {
        return ColorType::COLOR_BLUE;
    }

    // Black: All components dark
    if (r < 60 && g < 60 && b < 60) {
        return ColorType::COLOR_BLACK;
    }

    return ColorType::COLOR_NONE;
}

void SensorDataBuffer::update_color_history(ColorType color) {
    colorHistory[colorHistoryIndex] = color;
    colorHistoryIndex = (colorHistoryIndex + 1) % 5;
}

bool SensorDataBuffer::check_color_consistency(ColorType targetColor) {
    // Count how many of the last 5 classifications match the target
    uint8_t matchCount = 0;
    for (int i = 0; i < 5; i++) {
        if (colorHistory[i] == targetColor) {
            matchCount++;
        }
    }
    return matchCount >= 4; // Need 4 out of 5 to confirm
}

bool SensorDataBuffer::is_object_red() {
    timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_RED);
}

bool SensorDataBuffer::is_object_green() {
    timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_GREEN);
}

bool SensorDataBuffer::is_object_blue() {
    timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_BLUE);
}

bool SensorDataBuffer::is_object_white() {
    timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_WHITE);
}

bool SensorDataBuffer::is_object_black() {
    timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_BLACK);
}

bool SensorDataBuffer::is_object_yellow() {
    timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_YELLOW);
}
