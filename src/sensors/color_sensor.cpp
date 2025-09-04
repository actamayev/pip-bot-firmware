#include "utils/config.h"
#include "color_sensor.h"

bool ColorSensor::initialize() {
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

    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < DELAY_BETWEEN_READINGS) return;
    
    // Read current sensor data (rate controlled by 50ms task delay ~20Hz)
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
    analogWrite(COLOR_SENSOR_LED_PIN, COLOR_SENSOR_LED_BRIGHTNESS);
    
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

void ColorSensor::printCalibrationValues() {
    SerialQueueManager::getInstance().queueMessage("Calibration Values:");
    SerialQueueManager::getInstance().queueMessage("Black point:");
    String blackMsg = "R: " + String(calibration.blackRed) + ", G: " + String(calibration.blackGreen) + ", B: " + String(calibration.blackBlue);
    SerialQueueManager::getInstance().queueMessage(blackMsg.c_str());
    
    SerialQueueManager::getInstance().queueMessage("White point:");
    String whiteMsg = "R: " + String(calibration.whiteRed) + ", G: " + String(calibration.whiteGreen) + ", B: " + String(calibration.whiteBlue);
    SerialQueueManager::getInstance().queueMessage(whiteMsg.c_str());
}

void ColorSensor::calibrateBlackPoint() {
    if (!isInitialized || !sensorConnected) return;
    
    SerialQueueManager::getInstance().queueMessage("Calibrating black point - ensure dark surface...");
    
    analogWrite(COLOR_SENSOR_LED_PIN, COLOR_SENSOR_LED_BRIGHTNESS);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for sensor to stabilize

    // Take multiple readings and average them
    const int numReadings = 5;
    uint32_t sumRed = 0, sumGreen = 0, sumBlue = 0;
    
    for (int i = 0; i < numReadings; i++) {
        sumRed += Veml3328.getRed();
        sumGreen += Veml3328.getGreen();
        sumBlue += Veml3328.getBlue();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    calibration.blackRed = sumRed / numReadings;
    calibration.blackGreen = sumGreen / numReadings;
    calibration.blackBlue = sumBlue / numReadings;
    
    SerialQueueManager::getInstance().queueMessage("Black point calibrated!");
    printCalibrationValues();
    analogWrite(COLOR_SENSOR_LED_PIN, 0);
}

void ColorSensor::calibrateWhitePoint() {
    if (!isInitialized || !sensorConnected) return;
    
    SerialQueueManager::getInstance().queueMessage("Calibrating white point - ensure white surface...");
    
    analogWrite(COLOR_SENSOR_LED_PIN, COLOR_SENSOR_LED_BRIGHTNESS);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for sensor to stabilize
    
    // Take multiple readings and average them
    const int numReadings = 5;
    uint32_t sumRed = 0, sumGreen = 0, sumBlue = 0;
    
    for (int i = 0; i < numReadings; i++) {
        sumRed += Veml3328.getRed();
        sumGreen += Veml3328.getGreen();
        sumBlue += Veml3328.getBlue();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    calibration.whiteRed = sumRed / numReadings;
    calibration.whiteGreen = sumGreen / numReadings;
    calibration.whiteBlue = sumBlue / numReadings;
    
    SerialQueueManager::getInstance().queueMessage("White point calibrated!");
    printCalibrationValues();
    isCalibrated = true;
    analogWrite(COLOR_SENSOR_LED_PIN, 0);
}

void ColorSensor::read_color_sensor() {
    // Use non-blocking state machine for maximum speed
    color_read_state_t state = Veml3328.readColorNonBlocking();
    
    // Only process when we have complete reading
    if (state != COLOR_STATE_COMPLETE) return;
    uint16_t red = Veml3328.getLastRed();
    uint16_t green = Veml3328.getLastGreen();
    uint16_t blue = Veml3328.getLastBlue();
    
    if (isCalibrated) {
        // Use calibrated normalization
        auto safeNormalize = [](uint16_t value, uint16_t black, uint16_t white) -> uint8_t {
            if (white <= black) return 0; // Invalid calibration
            int32_t normalized = ((int32_t)(value - black) * 255L) / (white - black);
            return (uint8_t)constrain(normalized, 0, 255);
        };
        
        colorSensorData.redValue = safeNormalize(red, calibration.blackRed, calibration.whiteRed);
        colorSensorData.greenValue = safeNormalize(green, calibration.blackGreen, calibration.whiteGreen);
        colorSensorData.blueValue = safeNormalize(blue, calibration.blackBlue, calibration.whiteBlue);
    } else {
        // Fallback to simple 8-bit conversion if not calibrated
        colorSensorData.redValue = (uint8_t)(red >> 8);
        colorSensorData.greenValue = (uint8_t)(green >> 8);
        colorSensorData.blueValue = (uint8_t)(blue >> 8);
    }
}
