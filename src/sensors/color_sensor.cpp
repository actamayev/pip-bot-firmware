#include "utils/config.h"
#include "color_sensor.h"

bool ColorSensor::initialize() {
    pinMode(COLOR_SENSOR_LED_PIN, OUTPUT);
    
    sensorConnected = false;  // Assume not connected initially
    
    // Try to initialize the sensor
    lastUpdateTime = millis();
    if (Veml3328.begin()) {
        SerialQueueManager::getInstance().queueMessage("Warning: VEML3328 sensor not detected");
        return false;
    }
    sensorConnected = true;
    
    // Configure sensor
    Veml3328.setIntTime(time_50);
    Veml3328.setGain(gain_x1);
    Veml3328.setSensitivity(false);
    analogWrite(COLOR_SENSOR_LED_PIN, 155);
    precompute_inverse_matrix();
    return true;
}

void ColorSensor::precompute_inverse_matrix() {
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
    
    lastUpdateTime = millis();
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
    
    // SerialQueueManager::getInstance().queueMessage("Color: R:%u G:%u B:%u\r\n", normalizedRed, normalizedGreen, normalizedBlue);
    colorSensorData.redValue = normalizedRed;
    colorSensorData.greenValue = normalizedGreen;
    colorSensorData.blueValue = normalizedBlue;
}

// 2/27/25 TODO: only take sensor reading if it was more than 20ms ago
ColorSensorData ColorSensor::getSensorData() {
    if (!sensorConnected) {
        // SerialQueueManager::getInstance().queueMessage("Sensor not connected - returning default values");
        colorSensorData.redValue = 0;
        colorSensorData.greenValue = 0;
        colorSensorData.blueValue = 0;
        return colorSensorData;
    }
    read_color_sensor();
    // SerialQueueManager::getInstance().queueMessage("red%d\n", colorSensorData.redValue);
    // SerialQueueManager::getInstance().queueMessage("green%d\n", colorSensorData.greenValue);
    // SerialQueueManager::getInstance().queueMessage("blue%d\n", colorSensorData.blueValue);
    return colorSensorData;
}
