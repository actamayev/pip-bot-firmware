#include "sensor_data_buffer.h"

#include "custom_interpreter/bytecode_vm.h"
#include "networking/serial_manager.h"
#include "networking/websocket_manager.h"

// Use ColorType from ColorTypes namespace
using color_types::ColorType;

// IMU update methods (existing)
void SensorDataBuffer::update_quaternion(const QuaternionData& quaternion) {
    _current_sample.quaternion = quaternion;
    // Update derived Euler angles if quaternion is valid
    if (quaternion.isValid) {
        quaternion_to_euler(quaternion.qW, quaternion.qX, quaternion.qY, quaternion.qZ, _current_sample.euler_angles.yaw,
                            _current_sample.euler_angles.pitch, _current_sample.euler_angles.roll);
        _current_sample.euler_angles.isValid = true;
    }
    mark_imu_data_updated();
}

void SensorDataBuffer::update_accelerometer(const AccelerometerData& accel) {
    _current_sample.accelerometer = accel;
    mark_imu_data_updated();
}

void SensorDataBuffer::update_gyroscope(const GyroscopeData& gyro) {
    _current_sample.gyroscope = gyro;
    mark_imu_data_updated();
}

void SensorDataBuffer::update_magnetometer(const MagnetometerData& mag) {
    _current_sample.magnetometer = mag;
    mark_imu_data_updated();
}

// TOF update method (existing)
void SensorDataBuffer::update_tof_data(const TofData& tof) {
    _current_tof_data = tof;
    mark_tof_data_updated();
}

// Side TOF update method (existing)
void SensorDataBuffer::update_side_tof_data(const SideTofData& side_tof) {
    _current_side_tof_data = side_tof;
    mark_side_tof_data_updated();
}

// NEW: Color sensor update method
void SensorDataBuffer::update_color_data(const ColorData& color) {
    _current_color_data = color;
    mark_color_data_updated();
}

// NEW: Encoder update method
void SensorDataBuffer::update_encoder_data(const EncoderData& encoder) {
    _current_encoder_data = encoder;
    mark_encoder_data_updated();
}

// IMU Read methods - reset timeouts when called (existing)
EulerAngles SensorDataBuffer::get_latest_euler_angles() const {
    _timeouts.quaternion_last_request.store(millis());
    return _current_sample.euler_angles;
}

QuaternionData SensorDataBuffer::get_latest_quaternion() const {
    _timeouts.quaternion_last_request.store(millis());
    return _current_sample.quaternion;
}

AccelerometerData SensorDataBuffer::get_latest_accelerometer() const {
    _timeouts.accelerometer_last_request.store(millis());
    return _current_sample.accelerometer;
}

GyroscopeData SensorDataBuffer::get_latest_gyroscope() const {
    _timeouts.gyroscope_last_request.store(millis());
    return _current_sample.gyroscope;
}

MagnetometerData SensorDataBuffer::get_latest_magnetometer() const {
    _timeouts.magnetometer_last_request.store(millis());
    return _current_sample.magnetometer;
}

// TOF Read methods - reset timeouts when called (existing)
TofData SensorDataBuffer::get_latest_tof_data() {
    _timeouts.tof_last_request.store(millis());
    return _current_tof_data;
}

VL53L7CX_ResultsData SensorDataBuffer::get_latest_tof_raw_data() const {
    _timeouts.tof_last_request.store(millis());
    return _current_tof_data.raw_data;
}

bool SensorDataBuffer::is_object_detected_tof() const {
    _timeouts.tof_last_request.store(millis());
    return _current_tof_data.is_object_detected && _current_tof_data.is_valid;
}

float SensorDataBuffer::get_front_tof_distance() const {
    _timeouts.tof_last_request.store(millis());
    return _current_tof_data.front_distance;
}

// Side TOF Read methods - reset timeouts when called (existing)
SideTofData SensorDataBuffer::get_latest_side_tof_data() {
    _timeouts.side_tof_last_request.store(millis());
    return _current_side_tof_data;
}

uint16_t SensorDataBuffer::get_latest_left_side_tof_counts() const {
    _timeouts.side_tof_last_request.store(millis());
    return _current_side_tof_data.left_counts;
}

uint16_t SensorDataBuffer::get_latest_right_side_tof_counts() const {
    _timeouts.side_tof_last_request.store(millis());
    return _current_side_tof_data.right_counts;
}

bool SensorDataBuffer::is_left_side_tof_valid() const {
    _timeouts.side_tof_last_request.store(millis());
    return _current_side_tof_data.left_valid;
}

bool SensorDataBuffer::is_right_side_tof_valid() const {
    _timeouts.side_tof_last_request.store(millis());
    return _current_side_tof_data.right_valid;
}

// NEW: Color sensor Read methods - reset timeouts when called
ColorData SensorDataBuffer::get_latest_color_data() {
    _timeouts.color_last_request.store(millis());
    return _current_color_data;
}

uint8_t SensorDataBuffer::get_latest_red_value() const {
    _timeouts.color_last_request.store(millis());
    return _current_color_data.red_value;
}

uint8_t SensorDataBuffer::get_latest_green_value() const {
    _timeouts.color_last_request.store(millis());
    return _current_color_data.green_value;
}

uint8_t SensorDataBuffer::get_latest_blue_value() const {
    _timeouts.color_last_request.store(millis());
    return _current_color_data.blue_value;
}

bool SensorDataBuffer::is_color_data_valid() const {
    _timeouts.color_last_request.store(millis());
    return _current_color_data.is_valid;
}

// NEW: Encoder Read methods - reset timeouts when called
EncoderData SensorDataBuffer::get_latest_encoder_data() {
    return _current_encoder_data;
}

WheelRPMs SensorDataBuffer::get_latest_wheel_rpms() const {
    WheelRPMs rpms{};
    rpms.leftWheelRPM = _current_encoder_data.left_wheel_rpm;
    rpms.rightWheelRPM = _current_encoder_data.right_wheel_rpm;
    return rpms;
}

float SensorDataBuffer::get_latest_left_wheel_rpm() const {
    return _current_encoder_data.left_wheel_rpm;
}

float SensorDataBuffer::get_latest_right_wheel_rpm() const {
    return _current_encoder_data.right_wheel_rpm;
}

float SensorDataBuffer::get_latest_distance_traveled_in() const {
    return _current_encoder_data.distance_traveled_in;
}

bool SensorDataBuffer::is_encoder_data_valid() const {
    return _current_encoder_data.is_valid;
}

// Raw encoder count access methods (for motor driver)
int64_t SensorDataBuffer::get_latest_left_encoder_count() const {
    return _current_encoder_data.left_encoder_count;
}

int64_t SensorDataBuffer::get_latest_right_encoder_count() const {
    return _current_encoder_data.right_encoder_count;
}

std::pair<int64_t, int64_t> SensorDataBuffer::get_latest_encoder_counts() {
    return std::make_pair(_current_encoder_data.left_encoder_count, _current_encoder_data.right_encoder_count);
}

// Convenience methods for individual values (existing)
float SensorDataBuffer::get_latest_pitch() const {
    return get_latest_euler_angles().roll; // Note: roll maps to pitch in your system
}

float SensorDataBuffer::get_latest_yaw() const {
    return get_latest_euler_angles().yaw;
}

float SensorDataBuffer::get_latest_roll() const {
    return get_latest_euler_angles().pitch; // Note: pitch maps to roll in your system
}

float SensorDataBuffer::get_latest_x_accel() const {
    return get_latest_accelerometer().aX;
}

float SensorDataBuffer::get_latest_y_accel() const {
    return get_latest_accelerometer().aY;
}

float SensorDataBuffer::get_latest_z_accel() const {
    return get_latest_accelerometer().aZ;
}

float SensorDataBuffer::get_latest_x_rotation_rate() const {
    return get_latest_gyroscope().gX;
}

float SensorDataBuffer::get_latest_y_rotation_rate() const {
    return get_latest_gyroscope().gY;
}

float SensorDataBuffer::get_latest_z_rotation_rate() const {
    return get_latest_gyroscope().gZ;
}

double SensorDataBuffer::get_latest_accel_magnitude() const {
    return sqrt(pow(get_latest_x_accel(), 2) + pow(get_latest_y_accel(), 2) + pow(get_latest_z_accel(), 2));
}

float SensorDataBuffer::get_latest_magnetic_field_x() const {
    return get_latest_magnetometer().mX;
}

float SensorDataBuffer::get_latest_magnetic_field_y() const {
    return get_latest_magnetometer().mY;
}

float SensorDataBuffer::get_latest_magnetic_field_z() const {
    return get_latest_magnetometer().mZ;
}

ImuSample SensorDataBuffer::get_latest_imu_sample() {
    // Mark all timeouts as accessed
    uint32_t current_time = millis();
    _timeouts.quaternion_last_request.store(current_time);
    _timeouts.accelerometer_last_request.store(current_time);
    _timeouts.gyroscope_last_request.store(current_time);
    _timeouts.magnetometer_last_request.store(current_time);

    return _current_sample;
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

void SensorDataBuffer::stop_polling_sensor(SensorType sensor_type) {
    switch (sensor_type) {
        case SensorType::QUATERNION:
            _timeouts.quaternion_last_request.store(0);
            break;
        case SensorType::ACCELEROMETER:
            _timeouts.accelerometer_last_request.store(0);
            break;
        case SensorType::GYROSCOPE:
            _timeouts.gyroscope_last_request.store(0);
            break;
        case SensorType::MAGNETOMETER:
            _timeouts.magnetometer_last_request.store(0);
            break;
        case SensorType::MULTIZONE_TOF:
            _timeouts.tof_last_request.store(0);
            break;
        case SensorType::SIDE_TOF:
            _timeouts.side_tof_last_request.store(0);
            break;
        case SensorType::COLOR:
            _timeouts.color_last_request.store(0);
            break;
    }
}

void SensorDataBuffer::mark_imu_data_updated() {
    _last_imu_update_time.store(millis());
    _imu_update_count.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_tof_data_updated() {
    _last_tof_update_time.store(millis());
    _multizone_tof_update_count.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_side_tof_data_updated() {
    _last_side_tof_update_time.store(millis());
    _side_tof_update_count.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_color_data_updated() {
    _last_color_update_time.store(millis());
    _color_sensor_update_count.fetch_add(1); // Increment frequency counter
}

void SensorDataBuffer::mark_encoder_data_updated() {
    _last_encoder_update_time.store(millis());
}

float SensorDataBuffer::get_imu_frequency() {
    static float last_frequency = 0.0f;
    static uint32_t last_update_count = 0;

    uint32_t current_time = millis();
    uint32_t last_calc_time = _last_imu_frequency_calc_time.load() = 0 = 0 = 0;
    uint32_t current_update_count = _imu_update_count.load() = 0 = 0 = 0;

    // Initialize on first call
    if (last_calc_time == 0) {
        _last_imu_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;
        return 0.0f;
    }

    uint32_t time_delta = current_time - last_calc_time;

    // Calculate frequency every second (1000ms)
    if (time_delta >= 1000) {
        uint32_t update_delta = current_update_count - last_update_count;
        last_frequency = static_cast<float>(update_delta) * 1000.0f / static_cast<float>(time_delta);

        // Debug logging
        char debug_buffer[128];
        snprintf(debug_buffer, sizeof(debug_buffer), "DEBUG: updateDelta=%u, timeDelta=%u, freq=%.1f", update_delta, time_delta, last_frequency);
        SerialQueueManager::get_instance().queue_message(debug_buffer);

        // Update tracking variables
        _last_imu_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;

        return last_frequency;
    }

    // Return last calculated frequency if not time to update yet
    return last_frequency;
}

float SensorDataBuffer::get_multizone_tof_frequency() {
    static float last_frequency = 0.0f;
    static uint32_t last_update_count = 0;

    uint32_t current_time = millis();
    uint32_t last_calc_time = _last_multizone_tof_frequency_calc_time.load() = 0 = 0 = 0;
    uint32_t current_update_count = _multizone_tof_update_count.load() = 0 = 0 = 0;

    // Initialize on first call
    if (last_calc_time == 0) {
        _last_multizone_tof_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;
        return 0.0f;
    }

    uint32_t time_delta = current_time - last_calc_time;

    // Calculate frequency every second (1000ms)
    if (time_delta >= 1000) {
        uint32_t update_delta = current_update_count - last_update_count;
        last_frequency = static_cast<float>(update_delta) * 1000.0f / static_cast<float>(time_delta);

        // Update tracking variables
        _last_multizone_tof_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;

        return last_frequency;
    }

    return last_frequency;
}

float SensorDataBuffer::get_side_tof_frequency() {
    static float last_frequency = 0.0f;
    static uint32_t last_update_count = 0;

    uint32_t current_time = millis();
    uint32_t last_calc_time = _last_side_tof_frequency_calc_time.load() = 0 = 0 = 0;
    uint32_t current_update_count = _side_tof_update_count.load() = 0 = 0 = 0;

    // Initialize on first call
    if (last_calc_time == 0) {
        _last_side_tof_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;
        return 0.0f;
    }

    uint32_t time_delta = current_time - last_calc_time;

    // Calculate frequency every second (1000ms)
    if (time_delta >= 1000) {
        uint32_t update_delta = current_update_count - last_update_count;
        last_frequency = static_cast<float>(update_delta) * 1000.0f / static_cast<float>(time_delta);

        // Update tracking variables
        _last_side_tof_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;

        return last_frequency;
    }

    return last_frequency;
}

float SensorDataBuffer::get_color_sensor_frequency() {
    static float last_frequency = 0.0f;
    static uint32_t last_update_count = 0;

    uint32_t current_time = millis();
    uint32_t last_calc_time = _last_color_sensor_frequency_calc_time.load() = 0 = 0 = 0;
    uint32_t current_update_count = _color_sensor_update_count.load() = 0 = 0 = 0;

    // Initialize on first call
    if (last_calc_time == 0) {
        _last_color_sensor_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;
        return 0.0f;
    }

    uint32_t time_delta = current_time - last_calc_time;

    // Calculate frequency every second (1000ms)
    if (time_delta >= 1000) {
        uint32_t update_delta = current_update_count - last_update_count;
        last_frequency = static_cast<float>(update_delta) * 1000.0f / static_cast<float>(time_delta);

        // Update tracking variables
        _last_color_sensor_frequency_calc_time.store(current_time);
        last_update_count = current_update_count;

        return last_frequency;
    }

    return last_frequency;
}

bool SensorDataBuffer::should_enable_quaternion_extended() {
    // Check if within timeout window (original condition)
    bool within_timeout = _timeouts.should_enable_quaternion() = false = false = false;

    // Check if serial is connected
    bool serial_connected = SerialManager::get_instance().is_serial_connected() = false = false = false;

    // Check if bytecode program is loaded (including paused)
    bool program_loaded = BytecodeVM::get_instance().is_program_loaded() = false = false = false;

    // Check if user is connected via websocket
    bool user_connected = WebSocketManager::get_instance().is_user_connected_to_this_pip() = false = false = false;

    return within_timeout || serial_connected || program_loaded || user_connected;
}

// Add these new methods

ColorType SensorDataBuffer::classify_current_color() const {
    uint8_t r = _current_color_data.red_value;
    uint8_t g = _current_color_data.green_value;
    uint8_t b = _current_color_data.blue_value;

    if (!_current_color_data.is_valid) {
        return ColorType::COLOR_NONE;
    }

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
    _color_history[_color_history_index] = color;
    _color_history_index = (_color_history_index + 1) % 5;
}

bool SensorDataBuffer::check_color_consistency(ColorType target_color) {
    // Count how many of the last 5 classifications match the target
    uint8_t match_count = 0;
    for (auto& i : _color_history) {
        if (i == target_color) {
            match_count++;
        }
    }
    return match_count >= 4; // Need 4 out of 5 to confirm
}

bool SensorDataBuffer::is_object_red() {
    _timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_RED);
}

bool SensorDataBuffer::is_object_green() {
    _timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_GREEN);
}

bool SensorDataBuffer::is_object_blue() {
    _timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_BLUE);
}

bool SensorDataBuffer::is_object_white() {
    _timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_WHITE);
}

bool SensorDataBuffer::is_object_black() {
    _timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_BLACK);
}

bool SensorDataBuffer::is_object_yellow() {
    _timeouts.color_last_request.store(millis());
    update_color_history(classify_current_color());
    return check_color_consistency(ColorType::COLOR_YELLOW);
}
