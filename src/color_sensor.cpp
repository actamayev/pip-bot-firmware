#include "./include/config.h"
#include "./include/color_sensor.h"

ColorSensor colorSensor;

ColorSensor::ColorSensor() {
    pinMode(COLOR_SENSOR_LED_PIN, OUTPUT);
    digitalWrite(COLOR_SENSOR_LED_PIN, LOW);  // Start with LED off

    // Attempt to initialize the sensor with timeout
    // Wire.begin();  // Make sure I2C is initialized
    
    sensorConnected = false;  // Assume not connected initially
    
    // Try to initialize the sensor
    if (Veml3328.begin()) {
        Serial.println("Warning: VEML3328 sensor not detected");
    } else {
        sensorConnected = true;
        // Only configure if sensor is detected
        Veml3328.setIntTime(time_50);
        Veml3328.setGain(gain_x1);
        Veml3328.setSensitivity(false);
        digitalWrite(COLOR_SENSOR_LED_PIN, HIGH);  // Turn LED on only if sensor found
    }
    
    lastUpdateTime = millis();
}

void ColorSensor::read_color_sensor() {
    if (!sensorConnected) {
        // Return default values if no sensor
        colorSensorData.redValue = 0;
        colorSensorData.greenValue = 0;
        colorSensorData.blueValue = 0;
        return;
    }
    if (!isCalibrated) {
        Serial.println("Please calibrate black and white points first!");
        return;
    }
    // digitalWrite(COLOR_SENSOR_LED_PIN, HIGH);
    // TODO: Consider removing this delay (use a state machine instead)
    // delay(60);
    
    uint16_t red = Veml3328.getRed();
    uint16_t green = Veml3328.getGreen();
    uint16_t blue = Veml3328.getBlue();

    // Normalize readings to 0-255 range
    uint8_t normalizedRed = map(red, calibration.blackRed, calibration.whiteRed, 0, 255);
    uint8_t normalizedGreen = map(green, calibration.blackGreen, calibration.whiteGreen, 0, 255);
    uint8_t normalizedBlue = map(blue, calibration.blackBlue, calibration.whiteBlue, 0, 255);
    
    colorSensorData.redValue = normalizedRed;
    colorSensorData.greenValue = normalizedGreen;
    colorSensorData.blueValue = normalizedBlue;

    Serial.println("\nNormalized readings (0-255):");
    Serial.printf("Red: %u\r\n", normalizedRed);
    Serial.printf("Green: %u\r\n", normalizedGreen);
    Serial.printf("Blue: %u\r\n", normalizedBlue);
    
    // Also show raw values
    Serial.println("\nRaw readings:");
    Serial.printf("Red: %u\r\n", (uint16_t)(red / scaleFactor));
    Serial.printf("Green: %u\r\n", (uint16_t)(green / scaleFactor));
    Serial.printf("Blue: %u\r\n", (uint16_t)(blue / scaleFactor));
    // digitalWrite(COLOR_SENSOR_LED_PIN, LOW);
}

// TODO: only take sensor reading if it was more than 20ms ago
ColorSensorData ColorSensor::getSensorData() {
    if (!sensorConnected) {
        Serial.println("Sensor not connected - returning default values");
        colorSensorData.redValue = 0;
        colorSensorData.greenValue = 0;
        colorSensorData.blueValue = 0;
        return colorSensorData;
    }
    read_color_sensor();
    Serial.printf("red%d\n", colorSensorData.redValue);
    Serial.printf("green%d\n", colorSensorData.greenValue);
    Serial.printf("blue%d\n", colorSensorData.blueValue);
    return colorSensorData;
}
