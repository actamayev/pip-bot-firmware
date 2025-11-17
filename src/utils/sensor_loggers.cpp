#include "sensor_loggers.h"

void imu_logger() {
    static uint32_t lastImuPrintTime = 0;
    const uint32_t IMU_PRINT_INTERVAL = 1000; // Print frequency every 1 second

    // Print frequency report every second
    if (millis() - lastImuPrintTime < IMU_PRINT_INTERVAL) return;

    // Debug: Check if we're getting IMU data at all
    EulerAngles eulerAngles = SensorDataBuffer::get_instance().get_latest_euler_angles();
    float frequency = SensorDataBuffer::get_instance().get_imu_frequency();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "IMU Frequency: %.1f Hz, Data Valid: %s, Yaw: %.1f, Pitch: %.1f, Roll: %.1f", frequency,
             eulerAngles.isValid ? "YES" : "NO", eulerAngles.yaw, eulerAngles.pitch, eulerAngles.roll);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastImuPrintTime = millis();
}

void side_tofs_logger() {
    static uint32_t lastPrintTime = 0;
    const uint32_t PRINT_INTERVAL = 50; // Print every 500ms

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    SideTofData tofCounts = SensorDataBuffer::get_instance().get_latest_side_tof_data();
    // DisplayScreen::get_instance().showDistanceSensors(tofCounts);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Left TOF: %u counts              || Right TOF: %u counts", tofCounts.left_counts, tofCounts.right_counts);
    SerialQueueManager::get_instance().queue_message(buffer);
    lastPrintTime = millis();
}

void setup_button_loggers() {
    Buttons::get_instance().set_left_button_click_handler(
        [](Button2& btn) { SerialQueueManager::get_instance().queue_message("Left Button clicked!"); });

    Buttons::get_instance().set_right_button_click_handler(
        [](Button2& btn) { SerialQueueManager::get_instance().queue_message("Right Button clicked!"); });

    Buttons::get_instance().set_left_button_long_press_handler([](Button2& btn) {
        SerialQueueManager::get_instance().queue_message("Left Button long pressed for " + String(btn.wasPressedFor()) + " ms");
    });

    Buttons::get_instance().set_right_button_long_press_handler([](Button2& btn) {
        SerialQueueManager::get_instance().queue_message("Right Button long pressed for " + String(btn.wasPressedFor()) + " ms");
    });
}

void multizone_tof_logger() {
    static uint32_t lastPrintTime = 0;
    const uint32_t PRINT_INTERVAL = 1000; // Print frequency every 1 second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    float frequency = SensorDataBuffer::get_instance().get_multizone_tof_frequency();
    TofData tofData = SensorDataBuffer::get_instance().get_latest_tof_data();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Multizone ToF Frequency: %.1f Hz, Data Valid: %s, Object Detected: %s", frequency,
             tofData.is_valid ? "YES" : "NO", tofData.is_object_detected ? "Object Detected" : "Object not detected");
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void side_tof_logger() {
    static uint32_t lastPrintTime = 0;
    const uint32_t PRINT_INTERVAL = 1000; // Print frequency every 1 second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    float frequency = SensorDataBuffer::get_instance().get_side_tof_frequency();
    SideTofData tofData = SensorDataBuffer::get_instance().get_latest_side_tof_data();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Side ToF Frequency: %.1f Hz, Left: %u, Right: %u", frequency, tofData.left_counts, tofData.right_counts);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void color_sensor_logger() {
    static uint32_t lastPrintTime = 0;
    const uint32_t PRINT_INTERVAL = 1000; // Print frequency every 1 second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    float frequency = SensorDataBuffer::get_instance().get_color_sensor_frequency();
    ColorData colorData = SensorDataBuffer::get_instance().get_latest_color_data();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Color Sensor Frequency: %.1f Hz, RGB: (%u,%u,%u)", frequency, colorData.red_value, colorData.green_value,
             colorData.blue_value);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void log_motor_rpm() {
    static uint32_t lastPrintTime = 0;
    const uint32_t PRINT_INTERVAL = 500;

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    auto rpms = SensorDataBuffer::get_instance().get_latest_wheel_rpms();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Motor RPM - Left: %.2f || Right: %.2f", rpms.leftWheelRPM, rpms.rightWheelRPM);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void display_performance_logger() {
    static uint32_t lastPrintTime = 0;
    const uint32_t PRINT_INTERVAL = 1000; // Print frequency every 1 second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    DisplayScreen& display = DisplayScreen::get_instance();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "DISPLAY: %.2f Hz actual I2C (Generated: %lu, Updates: %lu, Skipped: %lu)", display.get_display_update_rate(),
             display.get_content_generation_count(), display.get_display_update_count(), display.get_skipped_update_count());
    SerialQueueManager::get_instance().queue_message(buffer);

    // Reset performance counters for next measurement period
    display.reset_performance_counters();
    lastPrintTime = millis();
}
