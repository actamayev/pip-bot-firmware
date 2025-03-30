#include "./motor_driver.h"
#include "../sensors/sensors.h"

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

// TODO 3/27/25: Change all analog writes with ledcWrite
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
    // Don't start new haptic feedback if we're already in a haptic sequence
    if (_hapticState != HAPTIC_IDLE) return;
    
    // Store direction for use in all stages
    _hapticDirection = direction;
    
    // Simplified sequence with more distinct feel
    // Calculate strengths as percentage of the input strength
    _resistanceStrength = min(255, (int)(strength * 1.2));  // Initial resistance
    _reversePulseStrength = min(255, (int)(strength * 0.9)); // Strong reverse pulse
    _centeringStrength = min(255, (int)(strength * 0.5));   // Gentle centering
    _finalBumpStrength = min(255, (int)(strength * 0.7));   // Definitive final "click"
    
    // Set durations for distinct feeling stages
    // Total duration will be longer than input duration to make effect more noticeable
    _resistanceDuration = min(255, (int)(duration_ms * 0.6)); // Initial resistance
    _reversePulseDuration = min(255, (int)(duration_ms * 0.3)); // Quick reverse pulse
    _centeringDuration = min(255, (int)(duration_ms * 0.8));  // Longer centering phase
    _finalBumpDuration = min(255, (int)(duration_ms * 0.4));  // Brief final bump
    _recoveryDuration = 50;  // Fixed recovery time before accepting new input
    
    // Start first stage - resistance against movement
    _hapticState = HAPTIC_RESISTANCE;
    _hapticStartTime = millis();
    
    // Apply resistance in the opposite direction of movement
    if (direction > 0) {
        right_motor_backward(_resistanceStrength);
    } else {
        right_motor_forward(_resistanceStrength);
    }
    
    Serial.printf("Haptic feedback: dir=%d, str=%d, dur=%d\n", 
                 direction, strength, duration_ms);
}

void MotorDriver::update_haptic_feedback() {
    if (_hapticState == HAPTIC_IDLE) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _hapticStartTime;
    
    switch (_hapticState) {
        case HAPTIC_RESISTANCE:
            if (elapsedTime >= _resistanceDuration) {
                _hapticState = HAPTIC_REVERSE_PULSE;
                _hapticStartTime = currentTime;
                
                // Apply pulse in same direction as user movement
                if (_hapticDirection > 0) {
                    right_motor_forward(_reversePulseStrength);
                } else {
                    right_motor_backward(_reversePulseStrength);
                }
            }
            break;
            
        case HAPTIC_REVERSE_PULSE:
            if (elapsedTime >= _reversePulseDuration) {
                _hapticState = HAPTIC_CENTERING;
                _hapticStartTime = currentTime;
                
                // Apply gentle centering force
                if (_hapticDirection > 0) {
                    right_motor_backward(_centeringStrength);
                } else {
                    right_motor_forward(_centeringStrength);
                }
            }
            break;
            
        case HAPTIC_CENTERING:
            if (elapsedTime >= _centeringDuration) {
                _hapticState = HAPTIC_FINAL_BUMP;
                _hapticStartTime = currentTime;
                
                // Final "bump" to create a definitive feel
                if (_hapticDirection > 0) {
                    right_motor_forward(_finalBumpStrength);
                } else {
                    right_motor_backward(_finalBumpStrength);
                }
            }
            break;
            
        case HAPTIC_FINAL_BUMP:
            if (elapsedTime >= _finalBumpDuration) {
                // Stop the motor completely
                right_motor_stop();
                
                // Enter recovery state (prevents immediate re-triggering)
                _hapticState = HAPTIC_RECOVERY;
                _hapticStartTime = currentTime;
            }
            break;
            
        case HAPTIC_RECOVERY:
            if (elapsedTime >= _recoveryDuration) {
                // Make sure motor is stopped before exiting sequence
                right_motor_stop();
                _hapticState = HAPTIC_IDLE;
            }
            break;
            
        default:
            _hapticState = HAPTIC_IDLE;
            right_motor_stop();
            break;
    }
}

void MotorDriver::set_motor_speeds(int16_t leftTarget, int16_t rightTarget) {
    // Store target speeds but don't change actual speeds immediately
    _targetLeftSpeed = constrain(leftTarget, -255, 255);
    _targetRightSpeed = constrain(rightTarget, -255, 255);
}

void MotorDriver::update_motor_speeds() {
    unsigned long currentTime = millis();
    
    // Only update at specified intervals
    if (currentTime - _lastSpeedUpdateTime < SPEED_RAMP_INTERVAL) {
        return;
    }

    _lastSpeedUpdateTime = currentTime;
    bool speedsChanged = false;

    // Gradually ramp left motor speed toward target
    if (_currentLeftSpeed < _targetLeftSpeed) {
        _currentLeftSpeed = min(static_cast<int16_t>(_currentLeftSpeed + SPEED_RAMP_STEP), _targetLeftSpeed);
        speedsChanged = true;
    } else if (_currentLeftSpeed > _targetLeftSpeed) {
        _currentLeftSpeed = max(static_cast<int16_t>(_currentLeftSpeed - SPEED_RAMP_STEP), _targetLeftSpeed);
        speedsChanged = true;
    }
    
    // Gradually ramp right motor speed toward target
    if (_currentRightSpeed < _targetRightSpeed) {
        _currentRightSpeed = min(static_cast<int16_t>(_currentRightSpeed + SPEED_RAMP_STEP), _targetRightSpeed);
        speedsChanged = true;
    } else if (_currentRightSpeed > _targetRightSpeed) {
        _currentRightSpeed = max(static_cast<int16_t>(_currentRightSpeed - SPEED_RAMP_STEP), _targetRightSpeed);
        speedsChanged = true;
    }

    // If straight driving is enabled, it will handle the motor control
    if (_straightDrivingEnabled) {
        update_straight_driving();
        return;
    }

    // Only update motor controls if speeds have changed
    if (speedsChanged) {
        // Apply the current speeds
        if (_currentLeftSpeed == 0) {
            left_motor_stop();
        } else if (_currentLeftSpeed > 0) {
            left_motor_backward(_currentLeftSpeed);
        } else {
            left_motor_forward(-_currentLeftSpeed);
        }

        if (_currentRightSpeed == 0) {
            right_motor_stop();
        } else if (_currentRightSpeed > 0) {
            right_motor_backward(_currentRightSpeed);
        } else {
            right_motor_forward(-_currentRightSpeed);
        }
    }
}

void MotorDriver::enable_straight_driving() {
    _straightDrivingEnabled = true;
    
    // Reset the yaw buffer
    _yawBufferIndex = 0;
    _yawBufferCount = 0;
    
    // Reset integral error
    _integralError = 0.0f;
    
    // Fill the buffer with the current yaw reading to start
    float currentYaw = Sensors::getInstance().getYaw();
    for (uint8_t i = 0; i < YAW_BUFFER_SIZE; i++) {
        _yawBuffer[i] = currentYaw;
    }
    
    // Store the initial average yaw as our reference point
    _initialYaw = currentYaw;
    _lastYawError = 0.0f;
    
    Serial.printf("Straight driving enabled. Initial yaw: %.2f\n", _initialYaw);
}

void MotorDriver::disable_straight_driving() {
    _straightDrivingEnabled = false;
    Serial.println("Straight driving disabled");
}

void MotorDriver::update_straight_driving() {
    if (!_straightDrivingEnabled) return;

    // Only apply corrections if both motors are moving in the same direction
    if ((_targetLeftSpeed > 0 && _targetRightSpeed > 0) || 
        (_targetLeftSpeed < 0 && _targetRightSpeed < 0)) {
        
        // Get current yaw from the IMU
        float rawYaw = Sensors::getInstance().getYaw();
        
        // Reject outliers (sudden large changes in raw readings)
        static float lastRawYaw = rawYaw;
        float yawDelta = abs(shortestAnglePath(lastRawYaw, rawYaw));
        if (yawDelta > 90.0f && _yawBufferCount > 0) {
            // Skip this reading - likely an outlier
            Serial.printf("Rejecting outlier: %.2f (last: %.2f, delta: %.2f)\n", 
                        rawYaw, lastRawYaw, yawDelta);
        } else {
            // Add to buffer and get the filtered average
            _yawBuffer[_yawBufferIndex] = rawYaw;
            _yawBufferIndex = (_yawBufferIndex + 1) % YAW_BUFFER_SIZE;
            if (_yawBufferCount < YAW_BUFFER_SIZE) _yawBufferCount++;
            lastRawYaw = rawYaw;
        }
        
        // Calculate circular mean (proper way to average angles)
        float sumSin = 0.0f;
        float sumCos = 0.0f;
        for (uint8_t i = 0; i < _yawBufferCount; i++) {
            float angleRad = _yawBuffer[i] * PI / 180.0f;
            sumSin += sin(angleRad);
            sumCos += cos(angleRad);
        }
        float filteredYaw = atan2(sumSin, sumCos) * 180.0f / PI;
        
        // Calculate error using shortest path
        float yawError = shortestAnglePath(_initialYaw, filteredYaw);

        // Add deadband to ignore small errors
        if (abs(yawError) < 1.0f) {
            yawError = 0.0f;
            // Reset integral when in deadband to prevent buildup
            _integralError *= 0.9f;
        }
        
        // Calculate integral with anti-windup
        _integralError += yawError;
        if (_integralError > YAW_I_MAX) _integralError = YAW_I_MAX;
        if (_integralError < -YAW_I_MAX) _integralError = -YAW_I_MAX;
        
        // Reset integral when crossing zero
        if ((yawError > 0 && _lastYawError < 0) || (yawError < 0 && _lastYawError > 0)) {
            _integralError *= 0.5f;
        }
        
        // Calculate derivative with filtering
        float yawDerivative = yawError - _lastYawError;
        _lastYawError = yawError;
        
        // Limit derivative value to prevent spikes
        if (yawDerivative > 10.0f) yawDerivative = 10.0f;
        if (yawDerivative < -10.0f) yawDerivative = -10.0f;
    
        // Calculate correction using PID controller with reduced gains
        int16_t proportionalTerm = static_cast<int16_t>(YAW_P_GAIN * yawError);
        int16_t integralTerm = static_cast<int16_t>(YAW_I_GAIN * _integralError);
        int16_t derivativeTerm = static_cast<int16_t>(YAW_D_GAIN * yawDerivative);
        
        int16_t correction = proportionalTerm + integralTerm + derivativeTerm;
        
        // Limit correction rate of change (slew rate limiting)
        static int16_t lastCorrection = 0;
        int16_t maxChange = 20; // Maximum change per update
        
        if (correction - lastCorrection > maxChange) {
            correction = lastCorrection + maxChange;
        } else if (lastCorrection - correction > maxChange) {
            correction = lastCorrection - maxChange;
        }
        lastCorrection = correction;
        
        // Apply correction to motor speeds (rest remains the same)
        int16_t leftAdjustment = -correction;
        int16_t rightAdjustment = correction;
        
        // Forward motion: positive _targetSpeed means backward motor direction in your setup
        if (_targetLeftSpeed > 0) {
            leftAdjustment = -leftAdjustment;
            rightAdjustment = -rightAdjustment;
        }
        
        // Apply adjustments but preserve the average speed
        int16_t leftAdjusted = _currentLeftSpeed + leftAdjustment;
        int16_t rightAdjusted = _currentRightSpeed + rightAdjustment;
        
        // Constrain to valid range
        leftAdjusted = constrain(leftAdjusted, -255, 255);
        rightAdjusted = constrain(rightAdjusted, -255, 255);
        
        // Apply the corrected speeds directly to motors
        if (leftAdjusted == 0) {
            left_motor_stop();
        } else if (leftAdjusted > 0) {
            left_motor_backward(leftAdjusted);
        } else {
            left_motor_forward(-leftAdjusted);
        }
        
        if (rightAdjusted == 0) {
            right_motor_stop();
        } else if (rightAdjusted > 0) {
            right_motor_backward(rightAdjusted);
        } else {
            right_motor_forward(-rightAdjusted);
        }
        
        // Optional debugging
        Serial.printf("Raw: %.2f, Filtered: %.2f, Error: %.2f, P: %d, I: %d, D: %d, L: %d, R: %d\n", 
                    rawYaw, filteredYaw, yawError, proportionalTerm, integralTerm, 
                    derivativeTerm, leftAdjusted, rightAdjusted);
    }
}

float MotorDriver::addYawReadingAndGetAverage(float newYaw) {
    // Add new reading to the buffer
    _yawBuffer[_yawBufferIndex] = newYaw;
    
    // Update buffer index
    _yawBufferIndex = (_yawBufferIndex + 1) % YAW_BUFFER_SIZE;
    
    // Update count of valid readings
    if (_yawBufferCount < YAW_BUFFER_SIZE) {
        _yawBufferCount++;
    }
    
    // Calculate average from valid readings
    float sum = 0.0f;
    for (uint8_t i = 0; i < _yawBufferCount; i++) {
        sum += _yawBuffer[i];
    }
    
    return sum / _yawBufferCount;
}

float MotorDriver::normalizeAngle(float angle) {
    // Normalize angle to range [-180, 180]
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float MotorDriver::shortestAnglePath(float from, float to) {
    // Find the shortest angular distance between two angles
    float diff = normalizeAngle(to - from);
    return diff;
}
