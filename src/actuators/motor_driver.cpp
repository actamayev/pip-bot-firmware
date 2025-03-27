#include "./motor_driver.h"
#include "../utils/config.h"

MotorDriver motorDriver;

MotorDriver::MotorDriver() {
    // Initialize motor pins
    pinMode(LEFT_MOTOR_PIN_IN_1, OUTPUT);
    pinMode(LEFT_MOTOR_PIN_IN_2, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_IN_1, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_IN_2, OUTPUT);
}

void MotorDriver::both_motors_forward(uint8_t speed) {
    left_motor_forward(speed);
    right_motor_forward(speed);
}

void MotorDriver::both_motors_backward(uint8_t speed) {
    left_motor_backward(speed);
    right_motor_backward(speed);
}

void MotorDriver::stop_both_motors() {
    left_motor_stop();
    right_motor_stop();
}

void MotorDriver::rotate_clockwise(uint8_t speed) {
    left_motor_forward(speed);
    right_motor_backward(speed);
}

void MotorDriver::rotate_counterclockwise(uint8_t speed) {
    left_motor_backward(speed);
    right_motor_forward(speed);
}

void MotorDriver::left_motor_forward(uint8_t speed) {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 0); // Explicitly clear backward pin
    digitalWrite(LEFT_MOTOR_PIN_IN_1, LOW);
    analogWrite(LEFT_MOTOR_PIN_IN_2, speed);
}

void MotorDriver::left_motor_backward(uint8_t speed) {
    analogWrite(LEFT_MOTOR_PIN_IN_2, 0); // Explicitly clear forward pin
    digitalWrite(LEFT_MOTOR_PIN_IN_2, LOW);
    analogWrite(LEFT_MOTOR_PIN_IN_1, speed);
}

void MotorDriver::left_motor_stop() {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 0);
    analogWrite(LEFT_MOTOR_PIN_IN_2, 0);
}

void MotorDriver::right_motor_forward(uint8_t speed) {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 0); // Explicitly clear backward pin
    digitalWrite(RIGHT_MOTOR_PIN_IN_1, LOW);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, speed);
}

void MotorDriver::right_motor_backward(uint8_t speed) {
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 0); // Explicitly clear forward pin
    digitalWrite(RIGHT_MOTOR_PIN_IN_2, LOW);
    analogWrite(RIGHT_MOTOR_PIN_IN_1, speed);
}

void MotorDriver::right_motor_stop() {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 0);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 0);
}

// In motor_driver.cpp
void MotorDriver::start_haptic_feedback(uint8_t strength, uint8_t duration_ms) {
    if (_hapticState != HAPTIC_IDLE) return;
    _hapticState = HAPTIC_PULSE_ACTIVE;
    _hapticStrength = strength;
    _hapticStartTime = millis();
    _hapticDuration = duration_ms;
    
    // Start the pulse (in opposite direction to create resistance)
    right_motor_backward(strength);
    
    Serial.printf("Haptic feedback started: strength=%d, duration=%d\n", 
                strength, duration_ms);
}

void MotorDriver::update_haptic_feedback() {
    // Only process if we're in an active haptic state
    if (_hapticState == HAPTIC_IDLE) return;

    unsigned long currentTime = millis();

    if (_hapticState == HAPTIC_PULSE_ACTIVE) {
        // Check if pulse duration has elapsed
        if (currentTime - _hapticStartTime >= _hapticDuration) {
            // Stop the motor
            right_motor_stop();
            
            // Enter recovery state (short pause before accepting new haptic commands)
            _hapticState = HAPTIC_PULSE_RECOVERY;
            _hapticStartTime = currentTime;
        }
    } else if (_hapticState == HAPTIC_PULSE_RECOVERY) {
        // Allow a short recovery time (10ms) before accepting new haptic commands
        if (currentTime - _hapticStartTime >= 10) {
            _hapticState = HAPTIC_IDLE;
        }
    }
}
