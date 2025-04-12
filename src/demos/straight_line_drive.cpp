#include "./straight_line_drive.h"

void StraightLineDrive::enable() {
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
    _lastRawYaw = 0.0f;

    Serial.printf("Straight driving enabled. Initial yaw: %.2f\n", _initialYaw);
}

void StraightLineDrive::disable() {
    _straightDrivingEnabled = false;
    Serial.println("Straight driving disabled");
}

void StraightLineDrive::update(int16_t& leftSpeed, int16_t& rightSpeed) {
    if (!_straightDrivingEnabled) return;

    // Only apply corrections if both motors are moving in the same direction
    if (!(leftSpeed > 0 && rightSpeed > 0)) return;
    // Get current yaw from the IMU
    float rawYaw = Sensors::getInstance().getYaw();

    // Reject outliers (sudden large changes in raw readings)
    float yawDelta = abs(shortestAnglePath(_lastRawYaw, rawYaw));
    if (yawDelta > 90.0f && _yawBufferCount > 0) {
        // Skip this reading - likely an outlier
        Serial.printf("Rejecting outlier: %.2f (last: %.2f, delta: %.2f)\n", 
                    rawYaw, _lastRawYaw, yawDelta);
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

    // Calculate correction using PID controller
    int16_t proportionalTerm = static_cast<int16_t>(YAW_P_GAIN * yawError);
    int16_t integralTerm = static_cast<int16_t>(YAW_I_GAIN * _integralError);
    int16_t derivativeTerm = static_cast<int16_t>(YAW_D_GAIN * yawDerivative);
    
    int16_t correction = proportionalTerm + integralTerm + derivativeTerm;
    
    // Limit correction rate of change (slew rate limiting)
    if (correction - _lastCorrection > MAX_CORRECTION_PER_CYCLE) {
        correction = _lastCorrection + MAX_CORRECTION_PER_CYCLE;
    } else if (_lastCorrection - correction > MAX_CORRECTION_PER_CYCLE) {
        correction = _lastCorrection - MAX_CORRECTION_PER_CYCLE;
    }
    _lastCorrection = correction;

    // Apply adjustments
    leftSpeed += correction;
    rightSpeed -= -correction;

    // Constrain to valid range
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);
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
