#include "../utils/config.h"
#include "./encoder_manager.h"

EncoderManager encoderManager;

EncoderManager::EncoderManager() {
    _leftWheelRPM = 0;
    _rightWheelRPM = 0;
    _lastUpdateTime = 0;
    Serial.println("Creating encoder manager");

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
    update();  // Update once for both wheels
    return {
        leftWheelRPM: _leftWheelRPM,
        rightWheelRPM: _rightWheelRPM
    };
}
