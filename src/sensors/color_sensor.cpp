#include "color_sensor.h"

bool ColorSensor::initialize() {
    pinMode(COLOR_SENSOR_LED_PIN, OUTPUT);

    _sensor_connected = false; // Assume not connected initially

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

            _sensor_connected = true;
            _is_initialized = true;
            SerialQueueManager::get_instance().queue_message("Color sensor initialized successfully");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    SerialQueueManager::get_instance().queue_message("Warning: VEML3328 sensor not detected");
    return false;
}

bool ColorSensor::should_be_polling() const {
    if (!_is_initialized) return false;

    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    // Continue polling if we should be enabled OR if sensor is currently enabled
    // (to allow proper cleanup when timeout expires)
    return timeouts.should_enable_color() || _sensor_enabled;
}

void ColorSensor::update_sensor_data() {
    if (!_is_initialized) return;

    // Check if we should enable/disable the sensor based on timeouts (ALWAYS check this first)
    ReportTimeouts& timeouts = SensorDataBuffer::get_instance().get_report_timeouts();
    bool should_enable = timeouts.should_enable_color();

    if (should_enable && !_sensor_enabled) {
        enable_color_sensor();
    } else if (!should_enable && _sensor_enabled) {
        disable_color_sensor();
        return; // Don't try to read data if sensor is disabled
    }

    if (!_sensor_enabled || !_sensor_connected) return; // Skip if sensor not enabled or connected

    uint32_t current_time = millis();
    if (current_time - _last_update_time < DELAY_BETWEEN_READINGS) return;

    // Read current sensor data (rate controlled by 50ms task delay ~20Hz)
    read_color_sensor();
    _last_update_time = current_time;

    // Create ColorData structure and write to buffer
    ColorData color_data;
    color_data.red_value = _color_sensor_data.redValue;
    color_data.green_value = _color_sensor_data.greenValue;
    color_data.blue_value = _color_sensor_data.blueValue;
    color_data.is_valid = _sensor_connected;
    color_data.timestamp = current_time;

    // Write to buffer
    SensorDataBuffer::get_instance().update_color_data(color_data);
}

void ColorSensor::enable_color_sensor() {
    if (!_is_initialized || _sensor_enabled) return;

    // Turn on LED for color sensor readings
    analogWrite(COLOR_SENSOR_LED_PIN, COLOR_SENSOR_LED_BRIGHTNESS);

    _sensor_enabled = true;
    SerialQueueManager::get_instance().queue_message("Color sensor enabled");
}

void ColorSensor::disable_color_sensor() {
    if (!_sensor_enabled) return;

    // Turn off LED to save power
    analogWrite(COLOR_SENSOR_LED_PIN, 0);

    _sensor_enabled = false;
    SerialQueueManager::get_instance().queue_message("Color sensor LED turned OFF - disabled due to timeout");
}

void ColorSensor::print_calibration_values() {
    SerialQueueManager::get_instance().queue_message("Calibration Values:");
    SerialQueueManager::get_instance().queue_message("Black point:");
    String black_msg = "R: " + String(_calibration.blackRed) + ", G: " + String(_calibration.blackGreen) + ", B: " + String(_calibration.blackBlue);
    SerialQueueManager::get_instance().queue_message(black_msg.c_str());

    SerialQueueManager::get_instance().queue_message("White point:");
    String white_msg = "R: " + String(_calibration.whiteRed) + ", G: " + String(_calibration.whiteGreen) + ", B: " + String(_calibration.whiteBlue);
    SerialQueueManager::get_instance().queue_message(white_msg.c_str());
}

void ColorSensor::calibrate_black_point() {
    if (!_is_initialized || !_sensor_connected) return;

    SerialQueueManager::get_instance().queue_message("Calibrating black point - ensure dark surface...");

    analogWrite(COLOR_SENSOR_LED_PIN, COLOR_SENSOR_LED_BRIGHTNESS);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for sensor to stabilize

    // Take multiple readings and average them
    const int NUM_READINGS = 5;
    uint32_t sum_red = 0;
    uint32_t sum_green = 0;
    uint32_t sum_blue = 0;

    for (int i = 0; i < NUM_READINGS; i++) {
        sum_red += Veml3328.getRed();
        sum_green += Veml3328.getGreen();
        sum_blue += Veml3328.getBlue();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    _calibration.blackRed = sum_red / NUM_READINGS;
    _calibration.blackGreen = sum_green / NUM_READINGS;
    _calibration.blackBlue = sum_blue / NUM_READINGS;

    SerialQueueManager::get_instance().queue_message("Black point calibrated!");
    print_calibration_values();
    analogWrite(COLOR_SENSOR_LED_PIN, 0);
}

void ColorSensor::calibrate_white_point() {
    if (!_is_initialized || !_sensor_connected) return;

    SerialQueueManager::get_instance().queue_message("Calibrating white point - ensure white surface...");

    analogWrite(COLOR_SENSOR_LED_PIN, COLOR_SENSOR_LED_BRIGHTNESS);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for sensor to stabilize

    // Take multiple readings and average them
    const int NUM_READINGS = 5;
    uint32_t sum_red = 0;
    uint32_t sum_green = 0;
    uint32_t sum_blue = 0;

    for (int i = 0; i < NUM_READINGS; i++) {
        sum_red += Veml3328.getRed();
        sum_green += Veml3328.getGreen();
        sum_blue += Veml3328.getBlue();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    _calibration.whiteRed = sum_red / NUM_READINGS;
    _calibration.whiteGreen = sum_green / NUM_READINGS;
    _calibration.whiteBlue = sum_blue / NUM_READINGS;

    SerialQueueManager::get_instance().queue_message("White point calibrated!");
    print_calibration_values();
    _is_calibrated = true;
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

    if (_is_calibrated) {
        // Use calibrated normalization
        auto safe_normalize = [](uint16_t value, uint16_t black, uint16_t white) -> uint8_t {
            if (white <= black) return 0; // Invalid calibration
            int32_t normalized = ((int32_t)(value - black) * 255L) / (white - black);
            return (uint8_t)constrain(normalized, 0, 255);
        };

        _color_sensor_data.redValue = safe_normalize(red, _calibration.blackRed, _calibration.whiteRed);
        _color_sensor_data.greenValue = safe_normalize(green, _calibration.blackGreen, _calibration.whiteGreen);
        _color_sensor_data.blueValue = safe_normalize(blue, _calibration.blackBlue, _calibration.whiteBlue);
    } else {
        // Fallback to simple 8-bit conversion if not calibrated
        _color_sensor_data.redValue = (uint8_t)(red >> 8);
        _color_sensor_data.greenValue = (uint8_t)(green >> 8);
        _color_sensor_data.blueValue = (uint8_t)(blue >> 8);
    }
}
