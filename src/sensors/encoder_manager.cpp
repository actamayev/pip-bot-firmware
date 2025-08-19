#include "utils/config.h"
#include "encoder_manager.h"

EncoderManager encoderManager;

EncoderManager::EncoderManager() {
    encoderMutex = xSemaphoreCreateMutex();
    _leftWheelRPM = 0;
    _rightWheelRPM = 0;
    _lastUpdateTime = 0;
    SerialQueueManager::getInstance().queueMessage("Creating encoder manager");

    // Initialize ESP32Encoder library
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    
    // Setup left encoder
    _leftEncoder.attachHalfQuad(LEFT_MOTOR_ENCODER_A, LEFT_MOTOR_ENCODER_B);
    _leftEncoder.clearCount();
    
    // Setup right encoder
    _rightEncoder.attachHalfQuad(RIGHT_MOTOR_ENCODER_A, RIGHT_MOTOR_ENCODER_B);
    _rightEncoder.clearCount();
    
    _lastUpdateTime = millis();
}

void EncoderManager::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _lastUpdateTime;
    
    // Only update if enough time has passed
    if (elapsedTime < RPM_CALC_INTERVAL) return;
    
    int64_t leftPulses = _leftEncoder.getCount();
    int64_t rightPulses = _rightEncoder.getCount();

    // Calculate motor shaft RPM - NOTE: Using elapsedTime in seconds
    float leftMotorShaftRPM = (float)(leftPulses * 60) / (ENCODER_CPR * (elapsedTime / 1000.0));
    float rightMotorShaftRPM = (float)(rightPulses * 60) / (ENCODER_CPR * (elapsedTime / 1000.0));

    // Calculate wheel RPM
    _leftWheelRPM = leftMotorShaftRPM / GEAR_RATIO;
    _rightWheelRPM = rightMotorShaftRPM / GEAR_RATIO;

    // Reset pulse counters for next interval
    _leftEncoder.clearCount();
    _rightEncoder.clearCount();

    _lastUpdateTime = currentTime;
}

WheelRPMs EncoderManager::getBothWheelRPMs() {
    // Lock mutex before accessing data
    if (xSemaphoreTake(encoderMutex, pdMS_TO_TICKS(MUTEX_REFRESH_FREQUENCY_MS)) == pdTRUE) {
        update();  // Update once for both wheels
        
        WheelRPMs result = {
            leftWheelRPM: _leftWheelRPM,
            rightWheelRPM: _rightWheelRPM
        };
        
        // Unlock mutex
        xSemaphoreGive(encoderMutex);
        return result;
    }

    // If mutex timeout, return last known values
    return {
        leftWheelRPM: _leftWheelRPM,
        rightWheelRPM: _rightWheelRPM
    };
}

void EncoderManager::resetDistanceTracking() {
    // Lock mutex before accessing encoder counts
    if (xSemaphoreTake(encoderMutex, pdMS_TO_TICKS(MUTEX_REFRESH_FREQUENCY_MS)) != pdTRUE) return;
    // Store current encoder counts as starting point
    _leftEncoderStartCount = _leftEncoder.getCount();
    _rightEncoderStartCount = _rightEncoder.getCount();
    
    // Unlock mutex
    xSemaphoreGive(encoderMutex);
}

float EncoderManager::getDistanceTraveledCm() {
    // Lock mutex before accessing encoder counts
    if (xSemaphoreTake(encoderMutex, pdMS_TO_TICKS(MUTEX_REFRESH_FREQUENCY_MS)) == pdTRUE) {
        // Get current encoder counts
        int64_t leftEncoderCurrentCount = _leftEncoder.getCount();
        int64_t rightEncoderCurrentCount = _rightEncoder.getCount();
        
        // Calculate change in encoder counts
        int64_t leftEncoderDelta = abs(leftEncoderCurrentCount - _leftEncoderStartCount);
        int64_t rightEncoderDelta = abs(rightEncoderCurrentCount - _rightEncoderStartCount);
        
        // Unlock mutex
        xSemaphoreGive(encoderMutex);
        
        // Average the two encoders to account for slight differences in wheel speeds
        float avgEncoderDelta = (leftEncoderDelta + rightEncoderDelta) / 2.0f;
        
        // Convert encoder counts to revolutions
        float wheelRevolutions = avgEncoderDelta / static_cast<float>(ENCODER_CPR);
        
        // Compensate for gear ratio
        float wheelRevolutionsAfterGearing = wheelRevolutions / GEAR_RATIO;
        
        // Convert wheel revolutions to distance traveled
        float distanceCm = wheelRevolutionsAfterGearing * WHEEL_CIRCUMFERENCE_CM;
        
        return distanceCm;
    }
    
    // If mutex timeout, return 0
    return 0.0f;
}
