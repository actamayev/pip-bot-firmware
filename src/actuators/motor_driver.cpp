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

void MotorDriver::start_haptic_feedback(int8_t direction, uint8_t strength, uint8_t duration_ms) {
    if (_hapticState != HAPTIC_IDLE) return;
    // Store direction for use in all stages
    _hapticDirection = direction;
    
    // Set strength for each stage (proportional to base strength)
    _resistanceStrength = strength;
    _reversePulseStrength = strength * 0.7;  // Slightly weaker reverse pulse
    _centeringStrength = strength * 0.4;     // Much gentler centering force
    
    // Set duration for each stage
    _resistanceDuration = duration_ms;          // Initial resistance
    _reversePulseDuration = duration_ms * 0.4;  // Shorter reverse pulse
    _centeringDuration = duration_ms * 0.8;     // Longer centering period
    _recoveryDuration = 15;                     // Fixed recovery time
    
    // Start first stage - resistance against movement
    _hapticState = HAPTIC_RESISTANCE;
    _hapticStartTime = millis();
    
    // Apply resistance in the opposite direction of movement
    if (direction > 0) {
        // User turning clockwise, apply counterclockwise resistance
        right_motor_backward(_resistanceStrength);
    } else {
        // User turning counterclockwise, apply clockwise resistance
        right_motor_forward(_resistanceStrength);
    }
    
    Serial.printf("Haptic sequence: dir=%d, strength=%d\n", direction, strength);
}

void MotorDriver::update_haptic_feedback() {
    // Only process if we're in an active haptic state
    if (_hapticState == HAPTIC_IDLE) return;

    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _hapticStartTime;

    switch (_hapticState) {
        case HAPTIC_RESISTANCE:
            // Check if resistance stage completed
            if (elapsedTime >= _resistanceDuration) {
                // Start reverse pulse stage - helping force in the direction of movement
                _hapticState = HAPTIC_REVERSE_PULSE;
                _hapticStartTime = currentTime;
                
                // Apply pulse in same direction as user movement (helping force)
                if (_hapticDirection > 0) {
                    // User turning clockwise, apply clockwise pulse
                    right_motor_forward(_reversePulseStrength);
                } else {
                    // User turning counterclockwise, apply counterclockwise pulse
                    right_motor_backward(_reversePulseStrength);
                }
            }
            break;

        case HAPTIC_REVERSE_PULSE:
            // Check if reverse pulse stage completed
            if (elapsedTime >= _reversePulseDuration) {
                // Start centering stage - gentle force toward centered position
                _hapticState = HAPTIC_CENTERING;
                _hapticStartTime = currentTime;
                
                // Apply gentle force toward "center" of the detent
                // (same direction as user was turning)
                if (_hapticDirection > 0) {
                    // User was turning clockwise, gentle clockwise centering
                    right_motor_forward(_centeringStrength);
                } else {
                    // User was turning counterclockwise, gentle counterclockwise centering
                    right_motor_backward(_centeringStrength);
                }
            }
            break;
            
        case HAPTIC_CENTERING:
            // Check if centering stage completed
            if (elapsedTime >= _centeringDuration) {
                // Stop the motor
                right_motor_stop();
                
                // Enter recovery state before accepting new haptic commands
                _hapticState = HAPTIC_RECOVERY;
                _hapticStartTime = currentTime;
            }
            break;

        case HAPTIC_RECOVERY:
            // Check if recovery time has elapsed
            if (elapsedTime >= _recoveryDuration) {
                _hapticState = HAPTIC_IDLE;
            }
            break;
  
        default:
            // Unexpected state, reset to idle
            _hapticState = HAPTIC_IDLE;
            right_motor_stop();
            break;
    }
}
