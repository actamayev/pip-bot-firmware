#include "./balance_controller.h"
#include "../actuators/rgb_led.h"
#include "../actuators/motor_driver.h"

BalanceController::BalanceController() {}

void BalanceController::enable() {
    if (_balancingEnabled == BalanceStatus::BALANCED) return;
    // Starting balance mode
    _balancingEnabled = BalanceStatus::BALANCED;

    // Reset PID variables
    _errorSum = 0.0f;
    _lastError = 0.0f;
    _lastUpdateTime = millis();

    float currentAngle = Sensors::getInstance().getPitch();
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
    StraightLineDrive::getInstance().disable();

    // Set LED to indicate balancing mode
    rgbLed.set_led_green();
    
    Serial.println("Balance mode enabled");
}

void BalanceController::disable() {
    if (_balancingEnabled == BalanceStatus::UNBALANCED) return;
    _balancingEnabled = BalanceStatus::UNBALANCED;
    motorDriver.stop_both_motors();
    rgbLed.turn_led_off();
    Serial.println("Balance mode disabled");
}

void BalanceController::update() {
    if (_balancingEnabled != BalanceStatus::BALANCED) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < UPDATE_INTERVAL) {
        return; // Maintain update rate
    }
    _lastUpdateTime = currentTime;

    // Get current pitch
    float rawAngle = Sensors::getInstance().getPitch();
    float currentAngle = rawAngle;

    // Update safety monitoring buffer
    _safetyBuffer[_safetyBufferIndex] = rawAngle;
    _safetyBufferIndex = (_safetyBufferIndex + 1) % ANGLE_BUFFER_SIZE;
    if (_safetyBufferCount < ANGLE_BUFFER_SIZE) _safetyBufferCount++;
    
    // Calculate safety buffer average using circular mean from utils
    float safetyAverage = calculateCircularMean(_safetyBuffer, _safetyBufferCount);
    
    // Calculate control buffer average using circular mean from utils
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
        Serial.printf("Safety cutoff triggered: Avg Angle %.2f exceeds limits\n", safetyAverage);
        disable();
        return;
    } 

    // PID calculation
    float error = TARGET_ANGLE - currentAngle;
    float gyroRate = Sensors::getInstance().getYRotationRate();

    // DEADBAND CHECK - New code
    // If within deadband angle and rotation rate is low, stop motors
    if (abs(error) < DEADBAND_ANGLE && abs(gyroRate) < MAX_STABLE_ROTATION) {
        // Within deadband and stable - stop motors
        motorDriver.stop_both_motors();

        // Important: Still update last error for continuity
        _lastError = error;
        return;
    }

    // If not in deadband, continue with normal PID control
    float deltaTime = UPDATE_INTERVAL / 1000.0f; // Convert to seconds

    // Calculate motor power using PID formula
    float proportionalTerm = P_GAIN * error;
    float integralTerm = I_GAIN * _errorSum;
    float derivativeTerm = D_GAIN * -gyroRate; 
    float yAccel = Sensors::getInstance().getYAccel();
    float feedforwardTerm = FF_GAIN * yAccel;

    int16_t motorPower = constrain(
        (int16_t)(proportionalTerm + integralTerm + derivativeTerm + feedforwardTerm),
        -MAX_BALANCE_POWER, 
        MAX_BALANCE_POWER
    );
    
    // Apply motor power
    motorDriver.set_motor_speeds(motorPower, motorPower);
    motorDriver.update_motor_speeds();
    
    // Store error for next iteration
    _lastError = error;
    
    // Debug output
    static unsigned long lastDebugTime = 0;
    if (currentTime - lastDebugTime > 100) {
        Serial.printf("Bal: Raw: %.2f, Filtered: %.2f, Error: %.2f, P: %.2f, I: %.2f, D: %.2f, Power: %d\n",
                     rawAngle, currentAngle, error, 
                     proportionalTerm, integralTerm, derivativeTerm, motorPower);
        lastDebugTime = currentTime;
    }
}
