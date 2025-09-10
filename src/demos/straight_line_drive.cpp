#include "straight_line_drive.h"

void StraightLineDrive::enable() {
    _straightDrivingEnabled = true;
    
    // Reset the yaw buffer
    _yawBufferIndex = 0;
    _yawBufferCount = 0;
    
    // Reset integral error
    _integralError = 0.0f;
    
    // Fill the buffer with the current yaw reading to start
    float currentYaw = SensorDataBuffer::getInstance().getLatestYaw();
    for (uint8_t i = 0; i < YAW_BUFFER_SIZE; i++) {
        _yawBuffer[i] = currentYaw;
    }

    // Store the initial average yaw as our reference point
    _initialYaw = currentYaw;
    _lastYawError = 0.0f;
    _lastRawYaw = 0.0f;
}

void StraightLineDrive::disable() {
    _straightDrivingEnabled = false;
}

void StraightLineDrive::update(int16_t& leftSpeed, int16_t& rightSpeed) {
    if (!_straightDrivingEnabled) return;

    // Only apply corrections if both motors are moving in the same direction
    if (!(leftSpeed > 0 && rightSpeed > 0)) return;
    // Get current yaw from the IMU
    float rawYaw = SensorDataBuffer::getInstance().getLatestYaw();

    // Reject outliers (sudden large changes in raw readings)
    float yawDelta = abs(shortestAnglePath(_lastRawYaw, rawYaw));
    if (yawDelta > 90.0f && _yawBufferCount > 0) {
        // Skip this reading - likely an outlier
    } else {
        // Add to buffer and get the filtered average
        _yawBuffer[_yawBufferIndex] = rawYaw;
        _yawBufferIndex = (_yawBufferIndex + 1) % YAW_BUFFER_SIZE;
        if (_yawBufferCount < YAW_BUFFER_SIZE) _yawBufferCount++;
        _lastRawYaw = rawYaw;
    }
    
    // Calculate circular mean (proper way to average angles)
    float filteredYaw = calculateCircularMean(_yawBuffer, _yawBufferCount);
    
    // Calculate error using shortest path
    float yawError = shortestAnglePath(_initialYaw, filteredYaw);
    
    // Update debug info
    _debugInfo.initialYaw = _initialYaw;
    _debugInfo.currentYaw = filteredYaw;
    _debugInfo.yawError = yawError;

    // Add deadband to ignore small errors
    if (abs(yawError) < DEADBAND) {
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

    // Calculate correction using PID controller
    int16_t proportionalTerm = static_cast<int16_t>(YAW_P_GAIN * yawError);
    int16_t integralTerm = static_cast<int16_t>(YAW_I_GAIN * _integralError);
    int16_t derivativeTerm = static_cast<int16_t>(YAW_D_GAIN * yawDerivative);
    
    int16_t correction = proportionalTerm + integralTerm + derivativeTerm;
    
    // Calculate speed-proportional correction scaling to reduce oscillation at low speeds
    float avgSpeed = (abs(leftSpeed) + abs(rightSpeed)) / 2.0f;
    float speedRatio = avgSpeed / static_cast<float>(MAX_MOTOR_SPEED); // 0.0 to 1.0
    // Scale correction with minimum of 20% for very low speeds, full scaling at high speeds
    float scaledMaxCorrection = MAX_CORRECTION_PER_CYCLE * max(0.2f, speedRatio);
    
    // Limit correction rate of change (speed-proportional slew rate limiting)
    if (correction - _lastCorrection > scaledMaxCorrection) {
        correction = _lastCorrection + static_cast<int16_t>(scaledMaxCorrection);
    } else if (_lastCorrection - correction > scaledMaxCorrection) {
        correction = _lastCorrection - static_cast<int16_t>(scaledMaxCorrection);
    }
    _lastCorrection = correction;
    
    // Limit correction magnitude to prevent any motor from going negative
    // Keep a minimum forward speed of 10 PWM to ensure forward motion
    int16_t minMotorSpeed = min(leftSpeed, rightSpeed);
    int16_t maxAllowedCorrection = max(0, minMotorSpeed - MIN_FORWARD_SPEED);
    
    // Clamp correction to safe range
    correction = constrain(correction, -maxAllowedCorrection, maxAllowedCorrection);

    // Apply asymmetric corrections - only slow down the motor that's drifting
    // Keep the motor that should speed up at its original target speed
    int16_t originalLeftSpeed = leftSpeed;
    int16_t originalRightSpeed = rightSpeed;
    
    if (correction > 0) {
        // Robot drifting right, need to correct left
        // Keep left motor at target speed, slow down right motor
        leftSpeed = originalLeftSpeed;
        rightSpeed = originalRightSpeed - abs(correction);
    } else if (correction < 0) {
        // Robot drifting left, need to correct right  
        // Keep right motor at target speed, slow down left motor
        rightSpeed = originalRightSpeed;
        leftSpeed = originalLeftSpeed - abs(correction);
    }

    // Constrain to valid range
    leftSpeed = constrain(leftSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    rightSpeed = constrain(rightSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    
    // Update debug info with final speeds and correction
    _debugInfo.leftSpeed = leftSpeed;
    _debugInfo.rightSpeed = rightSpeed;
    _debugInfo.correction = correction;
    
    rgbLed.set_led_green();
}

float StraightLineDrive::normalizeAngle(float angle) {
    // Normalize angle to range [-180, 180]
    angle = fmod(angle + 180.0f, 360.0f);
    if (angle < 0) angle += 360.0f;
    return angle - 180.0f;
}

float StraightLineDrive::shortestAnglePath(float from, float to) {
    // Find the shortest angular distance between two angles
    float diff = normalizeAngle(to - from);
    return diff;
}
