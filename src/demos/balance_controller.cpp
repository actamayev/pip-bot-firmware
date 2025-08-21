#include "balance_controller.h"

void BalanceController::enable() {
    if (_balancingEnabled == BalanceStatus::BALANCED) return;
    // Starting balance mode
    _balancingEnabled = BalanceStatus::BALANCED;

    // Reset PID variables
    _errorSum = 0.0f;
    _lastError = 0.0f;
    _lastUpdateTime = millis();

    float currentAngle = SensorDataBuffer::getInstance().getLatestPitch();
    _lastValidAngle = currentAngle;

    // Initialize buffers with the current angle
    for (uint8_t i = 0; i < ANGLE_BUFFER_SIZE; i++) {
        _angleBuffer[i] = currentAngle;
        _safetyBuffer[i] = currentAngle;
    }
    _angleBufferIndex = 0;
    _angleBufferCount = ANGLE_BUFFER_SIZE;
    _safetyBufferIndex = 0;
    _safetyBufferCount = ANGLE_BUFFER_SIZE;

    // Disable straight driving correction
    // StraightLineDrive::getInstance().disable();

    // Set LED to indicate balancing mode
    // rgbLed.set_led_green();
}

void BalanceController::disable() {
    if (_balancingEnabled == BalanceStatus::UNBALANCED) return;
    _balancingEnabled = BalanceStatus::UNBALANCED;
    motorDriver.brake_if_moving();
    rgbLed.turn_all_leds_off();
    DemoManager::getInstance()._currentDemo = Demo::DemoType::NONE;
}

void BalanceController::update() {
    if (_balancingEnabled != BalanceStatus::BALANCED) return;

    // unsigned long currentTime = millis();
    // if (currentTime - _lastUpdateTime < UPDATE_INTERVAL) {
    //     return; // Maintain update rate
    // }
    // _lastUpdateTime = currentTime;

    // Get current pitch
    float rawAngle = SensorDataBuffer::getInstance().getLatestPitch();
    float currentAngle = rawAngle;

    // Update safety monitoring buffer
    _safetyBuffer[_safetyBufferIndex] = rawAngle;
    _safetyBufferIndex = (_safetyBufferIndex + 1) % ANGLE_BUFFER_SIZE;
    if (_safetyBufferCount < ANGLE_BUFFER_SIZE) _safetyBufferCount++;
    
    // Calculate safety buffer average using circular mean
    float safetyAverage = calculateCircularMean(_safetyBuffer, _safetyBufferCount);
    
    // Calculate control buffer average using circular mean
    float controlAverage = calculateCircularMean(_angleBuffer, _angleBufferCount);
    
    // Validate reading against control average for PID input
    float deviation = abs(rawAngle - controlAverage);
    if (deviation > MAX_SAFE_ANGLE_DEVIATION / 3 && _angleBufferCount > 0) {
        // Reading differs too much from average - use last valid reading
        currentAngle = _lastValidAngle;
    } else {
        // Reading is valid - update last valid angle and control buffer
        _lastValidAngle = rawAngle;
        
        // Add to angle buffer
        _angleBuffer[_angleBufferIndex] = rawAngle;
        _angleBufferIndex = (_angleBufferIndex + 1) % ANGLE_BUFFER_SIZE;
        if (_angleBufferCount < ANGLE_BUFFER_SIZE) _angleBufferCount++;
    }

    // Safety check
    if (abs(safetyAverage - TARGET_ANGLE) > MAX_SAFE_ANGLE_DEVIATION) {
        disable();
        return;
    }

    // PID calculation
    float error = TARGET_ANGLE - currentAngle;
    float gyroRate = SensorDataBuffer::getInstance().getLatestYRotationRate();

    // If within deadband angle and rotation rate is low, stop motors
    if (abs(error) < DEADBAND_ANGLE && abs(gyroRate) < MAX_STABLE_ROTATION) {
        // Within deadband and stable - stop motors
        motorDriver.stop_both_motors();

        // Important: Still update last error for continuity
        _lastError = error;
        return;
    }

    // Calculate motor power using PID formula
    float proportionalTerm = P_GAIN * error;
    float integralTerm = I_GAIN * _errorSum;
    float derivativeTerm = D_GAIN * -gyroRate; 
    // float yAccel = SensorDataBuffer::getInstance().getLatestYAccel();
    // float feedforwardTerm = FF_GAIN * yAccel;

    int16_t motorPower = constrain(
        (int16_t)(proportionalTerm + integralTerm + derivativeTerm),
        -MAX_BALANCE_POWER, 
        MAX_BALANCE_POWER
    );

    int adjustedPWM = motorPower;
    if (motorPower > 0) {
        adjustedPWM = constrain(motorPower, MIN_EFFECTIVE_PWM, MAX_MOTOR_SPEED);
    } else if (motorPower < 0) {
        adjustedPWM = constrain(motorPower, -MAX_MOTOR_SPEED, -MIN_EFFECTIVE_PWM);
    }

    // Apply motor power
    motorDriver.set_motor_speeds(adjustedPWM, adjustedPWM, false);

    // Store error for next iteration
    _lastError = error;
}

void BalanceController::updateBalancePids(NewBalancePids newBalancePids) {
    P_GAIN = newBalancePids.pValue;                    // 0-255
    I_GAIN = newBalancePids.iValue;                    // 0-255
    D_GAIN = newBalancePids.dValue;                    // 0-255
    FF_GAIN = newBalancePids.ffValue;                  // 0-255
    TARGET_ANGLE = newBalancePids.targetAngle;         // 0-255
    MAX_SAFE_ANGLE_DEVIATION = newBalancePids.maxSafeAngleDeviation; // 0-255
    UPDATE_INTERVAL = newBalancePids.updateInterval;   // 0-255
    DEADBAND_ANGLE = newBalancePids.deadbandAngle;     // 0-255
    MAX_STABLE_ROTATION = newBalancePids.maxStableRotation; // 0-255
    MIN_EFFECTIVE_PWM = newBalancePids.minEffectivePwm;
}
