#include "utils/config.h"
#include "color_sensor.h"

bool ColorSensor::canRetryInitialization() const {
    if (isInitialized) return false;

    unsigned long currentTime = millis();
    if (currentTime - lastInitAttempt < INIT_RETRY_INTERVAL) {
        return false; // Too soon to retry
    }

    if (initRetryCount >= MAX_INIT_RETRIES) {
        return false; // Too many retries
    }

    return true;
}

bool ColorSensor::initialize() {
    precompute_inverse_matrix();
    lastInitAttempt = millis();
    initRetryCount++;
    
    pinMode(COLOR_SENSOR_LED_PIN, OUTPUT);
    
    sensorConnected = false;  // Assume not connected initially
    
    // Add a delay before trying to initialize
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Try a few times with short delays in between
    for (int attempt = 0; attempt < 3; attempt++) {
        // Try to initialize the sensor with explicit I2C address and wire instance
        // Use 0x10 address explicitly and pass the already initialized Wire object
        if (Veml3328.begin(0x10, &Wire) == 0) {
            // Configure sensor
            Veml3328.setIntTime(time_50);
            Veml3328.setGain(gain_x1);
            Veml3328.setSensitivity(false);
            
            sensorConnected = true;
            isInitialized = true;
            SerialQueueManager::getInstance().queueMessage("Color sensor initialized successfully");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    SerialQueueManager::getInstance().queueMessage("Warning: VEML3328 sensor not detected");
    return false;
}

bool ColorSensor::shouldBePolling() const {
    if (!isInitialized) return false;
    
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    // Continue polling if we should be enabled OR if sensor is currently enabled
    // (to allow proper cleanup when timeout expires)
    return timeouts.shouldEnableColor() || sensorEnabled;
}

void ColorSensor::updateSensorData() {
    if (!isInitialized) return;

    // Check if we should enable/disable the sensor based on timeouts (ALWAYS check this first)
    ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
    bool shouldEnable = timeouts.shouldEnableColor();
    
    if (shouldEnable && !sensorEnabled) {
        enableColorSensor();
    } else if (!shouldEnable && sensorEnabled) {
        disableColorSensor();
        return; // Don't try to read data if sensor is disabled
    }
    
    if (!sensorEnabled || !sensorConnected) return; // Skip if sensor not enabled or connected
    
    // Rate limiting - only read if enough time has passed (but timeout check happens above)
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < DELAY_BETWEEN_READINGS) {
        return;
    }
    
    // Read current sensor data
    read_color_sensor();
    lastUpdateTime = currentTime;
    
    // Create ColorData structure and write to buffer
    ColorData colorData;
    colorData.redValue = colorSensorData.redValue;
    colorData.greenValue = colorSensorData.greenValue;
    colorData.blueValue = colorSensorData.blueValue;
    colorData.isValid = sensorConnected;
    colorData.timestamp = currentTime;
    
    // Write to buffer
    SensorDataBuffer::getInstance().updateColorData(colorData);
}

void ColorSensor::enableColorSensor() {
    if (!isInitialized || sensorEnabled) return;
    
    // Turn on LED for color sensor readings
    analogWrite(COLOR_SENSOR_LED_PIN, 155);
    
    sensorEnabled = true;
    SerialQueueManager::getInstance().queueMessage("Color sensor enabled");
}

void ColorSensor::disableColorSensor() {
    if (!sensorEnabled) return;
    
    // Turn off LED to save power
    analogWrite(COLOR_SENSOR_LED_PIN, 0);
    
    sensorEnabled = false;
    SerialQueueManager::getInstance().queueMessage("Color sensor LED turned OFF - disabled due to timeout");
}

void ColorSensor::precompute_inverse_matrix() {
    if (wasInverseMatrixPrecomputed) return;
    // Pre-compute the inverse matrix
    float matrix[3][3] = {
        {calibrationValues.redRedValue, calibrationValues.greenRedValue, calibrationValues.blueRedValue},
        {calibrationValues.redGreenValue, calibrationValues.greenGreenValue, calibrationValues.blueGreenValue},
        {calibrationValues.redBlueValue, calibrationValues.greenBlueValue, calibrationValues.blueBlueValue}
    };
      
    // Calculate the determinant of the matrix
    float det = 
    matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
    matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
    matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    
    // Calculate the inverse matrix
    float invDet = 1.0 / det;

    invMatrix[0][0] = (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) * invDet;
    invMatrix[0][1] = (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) * invDet;
    invMatrix[0][2] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) * invDet;
    
    invMatrix[1][0] = (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) * invDet;
    invMatrix[1][1] = (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) * invDet;
    invMatrix[1][2] = (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) * invDet;
    
    invMatrix[2][0] = (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) * invDet;
    invMatrix[2][1] = (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) * invDet;
    invMatrix[2][2] = (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) * invDet;
    wasInverseMatrixPrecomputed = true;
}

void ColorSensor::read_color_sensor() {
    // Get raw sensor readings
    float rawRed = Veml3328.getRed();
    float rawGreen = Veml3328.getGreen();
    float rawBlue = Veml3328.getBlue();
    
    // Apply pre-computed inverse matrix to get true RGB values
    float trueRed = invMatrix[0][0] * rawRed + invMatrix[0][1] * rawGreen + invMatrix[0][2] * rawBlue;
    float trueGreen = invMatrix[1][0] * rawRed + invMatrix[1][1] * rawGreen + invMatrix[1][2] * rawBlue;
    float trueBlue = invMatrix[2][0] * rawRed + invMatrix[2][1] * rawGreen + invMatrix[2][2] * rawBlue;
    
    // Handle negative values
    trueRed = max(0.0f, trueRed);
    trueGreen = max(0.0f, trueGreen);
    trueBlue = max(0.0f, trueBlue);
    
    // Normalize to percentage (0-100%)
    float maxVal = max(max(trueRed, trueGreen), trueBlue);
    float redPercent, greenPercent, bluePercent;
    
    if (maxVal > 0) {
      redPercent = (trueRed / maxVal) * 100.0;
      greenPercent = (trueGreen / maxVal) * 100.0;
      bluePercent = (trueBlue / maxVal) * 100.0;
    } else {
      redPercent = greenPercent = bluePercent = 0;
    }
    
    // Calculate normalized 0-255 RGB values
    uint8_t normalizedRed = (uint8_t)(redPercent * 2.55);
    uint8_t normalizedGreen = (uint8_t)(greenPercent * 2.55);
    uint8_t normalizedBlue = (uint8_t)(bluePercent * 2.55);
    
    colorSensorData.redValue = normalizedRed;
    colorSensorData.greenValue = normalizedGreen;
    colorSensorData.blueValue = normalizedBlue;
}
