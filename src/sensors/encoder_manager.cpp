#include "encoder_manager.h"

#include "utils/config.h"

EncoderManager::EncoderManager() : _leftWheelRPM(0), _rightWheelRPM(0), _lastUpdateTime(0), _isInitialized(false) {
    // Will be set to true in initialize()
    SerialQueueManager::get_instance().queue_message("Creating encoder manager");
}

bool EncoderManager::initialize() {
    if (_isInitialized) {
        return true;
    }

    SerialQueueManager::get_instance().queue_message("Initializing Encoder Manager...");

    // Initialize ESP32Encoder library
    ESP32Encoder::useInternalWeakPullResistors = puType::up;

    // Setup left encoder
    _leftEncoder.attachFullQuad(LEFT_MOTOR_ENCODER_A, LEFT_MOTOR_ENCODER_B);
    _leftEncoder.clearCount();

    // Setup right encoder
    _rightEncoder.attachFullQuad(RIGHT_MOTOR_ENCODER_A, RIGHT_MOTOR_ENCODER_B);
    _rightEncoder.clearCount();

    _lastUpdateTime = millis();
    _leftEncoderStartCount = 0;
    _rightEncoderStartCount = 0;
    _leftLastCount = 0;
    _rightLastCount = 0;

    _isInitialized = true;
    SerialQueueManager::get_instance().queue_message("Encoder Manager initialized successfully");
    return true;
}

void EncoderManager::update() {
    uint32_t current_time = millis();
    uint32_t elapsed_time = current_time - _lastUpdateTime;

    // Only update if enough time has passed
    if (elapsed_time < RPM_CALC_INTERVAL) {
        return;
    }

    int64_t left_current_count = _leftEncoder.getCount();
    int64_t right_current_count = _rightEncoder.getCount();

    // Calculate delta pulses since last update
    int64_t left_pulses = left_current_count - _leftLastCount;
    int64_t right_pulses = right_current_count - _rightLastCount;

    // Calculate motor shaft RPM - NOTE: Using elapsedTime in seconds
    float left_motor_shaft_rpm = static_cast<float>(left_pulses * 60) / (PULSES_PER_REVOLUTION * (elapsed_time / 1000.0));
    float right_motor_shaft_rpm = static_cast<float>(right_pulses * 60) / (PULSES_PER_REVOLUTION * (elapsed_time / 1000.0));

    // Calculate wheel RPM
    _leftWheelRPM = left_motor_shaft_rpm / GEAR_RATIO;
    _rightWheelRPM = right_motor_shaft_rpm / GEAR_RATIO;

    // Store current counts for next delta calculation
    _leftLastCount = left_current_count;
    _rightLastCount = right_current_count;

    _lastUpdateTime = current_time;
}

// Standard sensor interface methods
bool EncoderManager::should_be_polling() const {
    return _isInitialized;
}

void EncoderManager::update_sensor_data() {
    if (!_isInitialized) {
        return;
    }

    // Call internal update method to calculate RPMs
    update();

    // Calculate distance traveled inline and capture raw counts
    float distance_traveled = 0.0f;
    int64_t left_count = 0;
    int64_t right_count = 0;

    // Get current encoder counts (both for distance calculation and raw storage)
    int64_t left_encoder_current_count = _leftEncoder.getCount();
    int64_t right_encoder_current_count = _rightEncoder.getCount();

    // Store raw counts for motor driver
    left_count = left_encoder_current_count;
    right_count = right_encoder_current_count;

    // Calculate change in encoder counts for distance (maintain sign for direction)
    int64_t left_encoder_delta = left_encoder_current_count - _leftEncoderStartCount;
    int64_t right_encoder_delta = right_encoder_current_count - _rightEncoderStartCount;

    // Average the two encoders to account for slight differences in wheel speeds
    float avg_encoder_delta = (left_encoder_delta + right_encoder_delta) / 2.0f;

    // Convert encoder counts to revolutions
    float wheel_revolutions = avg_encoder_delta / static_cast<float>(PULSES_PER_REVOLUTION);

    // Compensate for gear ratio
    float wheel_revolutions_after_gearing = wheel_revolutions / GEAR_RATIO;

    // Convert wheel revolutions to distance traveled
    distance_traveled = wheel_revolutions_after_gearing * WHEEL_CIRCUMFERENCE_IN;

    // Create encoder data struct with both calculated and raw values
    EncoderData encoder_data;
    encoder_data.left_wheel_rpm = _leftWheelRPM;
    encoder_data.right_wheel_rpm = _rightWheelRPM;
    encoder_data.distance_traveled_in = distance_traveled;
    encoder_data.left_encoder_count = left_count;   // Raw counts for motor driver
    encoder_data.right_encoder_count = right_count; // Raw counts for motor driver
    encoder_data.is_valid = true;
    encoder_data.timestamp = millis();

    // Write to sensor data buffer
    SensorDataBuffer::get_instance().update_encoder_data(encoder_data);
}
