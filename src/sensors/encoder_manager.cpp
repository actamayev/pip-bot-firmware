#include "utils/config.h"
#include "encoder_manager.h"

EncoderManager::EncoderManager() {
    _leftWheelRPM = 0;
    _rightWheelRPM = 0;
    _lastUpdateTime = 0;
    isInitialized = false;  // Will be set to true in initialize()
    SerialQueueManager::getInstance().queueMessage("Creating encoder manager");
}

bool EncoderManager::initialize() {
    if (isInitialized) return true;
    
    SerialQueueManager::getInstance().queueMessage("Initializing Encoder Manager...");
    
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
    
    isInitialized = true;
    SerialQueueManager::getInstance().queueMessage("Encoder Manager initialized successfully");
    return true;
}

void EncoderManager::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _lastUpdateTime;
    
    // Only update if enough time has passed
    if (elapsedTime < RPM_CALC_INTERVAL) return;
    
    int64_t leftCurrentCount = _leftEncoder.getCount();
    int64_t rightCurrentCount = _rightEncoder.getCount();

    // Calculate delta pulses since last update
    int64_t leftPulses = leftCurrentCount - _leftLastCount;
    int64_t rightPulses = rightCurrentCount - _rightLastCount;

    // Calculate motor shaft RPM - NOTE: Using elapsedTime in seconds
    float leftMotorShaftRPM = (float)(leftPulses * 60) / (PULSES_PER_REVOLUTION * (elapsedTime / 1000.0));
    float rightMotorShaftRPM = (float)(rightPulses * 60) / (PULSES_PER_REVOLUTION * (elapsedTime / 1000.0));

    // Calculate wheel RPM
    _leftWheelRPM = leftMotorShaftRPM / GEAR_RATIO;
    _rightWheelRPM = rightMotorShaftRPM / GEAR_RATIO;

    // Store current counts for next delta calculation
    _leftLastCount = leftCurrentCount;
    _rightLastCount = rightCurrentCount;

    _lastUpdateTime = currentTime;
}

// Standard sensor interface methods
bool EncoderManager::shouldBePolling() const {
    return isInitialized;
}

void EncoderManager::updateSensorData() {
    if (!isInitialized) return;

    // Call internal update method to calculate RPMs
    update();
    
    // Calculate distance traveled inline and capture raw counts
    float distanceTraveled = 0.0f;
    int64_t leftCount = 0;
    int64_t rightCount = 0;
    
    // Get current encoder counts (both for distance calculation and raw storage)
    int64_t leftEncoderCurrentCount = _leftEncoder.getCount();
    int64_t rightEncoderCurrentCount = _rightEncoder.getCount();
    
    // Store raw counts for motor driver
    leftCount = leftEncoderCurrentCount;
    rightCount = rightEncoderCurrentCount;
    
    // Calculate change in encoder counts for distance (maintain sign for direction)
    int64_t leftEncoderDelta = leftEncoderCurrentCount - _leftEncoderStartCount;
    int64_t rightEncoderDelta = rightEncoderCurrentCount - _rightEncoderStartCount;
    
    // Average the two encoders to account for slight differences in wheel speeds
    float avgEncoderDelta = (leftEncoderDelta + rightEncoderDelta) / 2.0f;
    
    // Convert encoder counts to revolutions
    float wheelRevolutions = avgEncoderDelta / static_cast<float>(PULSES_PER_REVOLUTION);
    
    // Compensate for gear ratio
    float wheelRevolutionsAfterGearing = wheelRevolutions / GEAR_RATIO;
    
    // Convert wheel revolutions to distance traveled
    distanceTraveled = wheelRevolutionsAfterGearing * WHEEL_CIRCUMFERENCE_IN;
    
    // Create encoder data struct with both calculated and raw values
    EncoderData encoderData;
    encoderData.leftWheelRPM = _leftWheelRPM;
    encoderData.rightWheelRPM = _rightWheelRPM;
    encoderData.distanceTraveledIn = distanceTraveled;
    encoderData.leftEncoderCount = leftCount;    // Raw counts for motor driver
    encoderData.rightEncoderCount = rightCount;  // Raw counts for motor driver
    encoderData.isValid = true;
    encoderData.timestamp = millis();
    
    // Write to sensor data buffer
    SensorDataBuffer::getInstance().updateEncoderData(encoderData);
}
