#include "sensor_loggers.h"

void imuLogger() {
    static unsigned long lastImuPrintTime = 0;
    const unsigned long IMU_PRINT_INTERVAL = 1000; // Print frequency every 1 second

    // Print frequency report every second
    if (millis() - lastImuPrintTime < IMU_PRINT_INTERVAL) return;

    // Debug: Check if we're getting IMU data at all
    EulerAngles eulerAngles = SensorDataBuffer::get_instance().getLatestEulerAngles();
    float frequency = SensorDataBuffer::get_instance().getImuFrequency();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "IMU Frequency: %.1f Hz, Data Valid: %s, Yaw: %.1f, Pitch: %.1f, Roll: %.1f", frequency,
             eulerAngles.isValid ? "YES" : "NO", eulerAngles.yaw, eulerAngles.pitch, eulerAngles.roll);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastImuPrintTime = millis();
}

void sideTofsLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 50; // Print every 500ms

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    SideTofData tofCounts = SensorDataBuffer::get_instance().getLatestSideTofData();
    // DisplayScreen::get_instance().showDistanceSensors(tofCounts);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Left TOF: %u counts              || Right TOF: %u counts", tofCounts.leftCounts, tofCounts.rightCounts);
    SerialQueueManager::get_instance().queue_message(buffer);
    lastPrintTime = millis();
}

void setupButtonLoggers() {
    Buttons::get_instance().set_left_button_click_handler([](Button2& btn) { SerialQueueManager::get_instance().queue_message("Left Button clicked!"); });

    Buttons::get_instance().set_right_button_click_handler(
        [](Button2& btn) { SerialQueueManager::get_instance().queue_message("Right Button clicked!"); });

    Buttons::get_instance().set_left_button_long_press_handler([](Button2& btn) {
        SerialQueueManager::get_instance().queue_message("Left Button long pressed for " + String(btn.wasPressedFor()) + " ms");
    });

    Buttons::get_instance().set_right_button_long_press_handler([](Button2& btn) {
        SerialQueueManager::get_instance().queue_message("Right Button long pressed for " + String(btn.wasPressedFor()) + " ms");
    });
}

void multizoneTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    float frequency = SensorDataBuffer::get_instance().getMultizoneTofFrequency();
    TofData tofData = SensorDataBuffer::get_instance().getLatestTofData();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Multizone ToF Frequency: %.1f Hz, Data Valid: %s, Object Detected: %s", frequency,
             tofData.isValid ? "YES" : "NO", tofData.isObjectDetected ? "Object Detected" : "Object not detected");
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void sideTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    float frequency = SensorDataBuffer::get_instance().getSideTofFrequency();
    SideTofData tofData = SensorDataBuffer::get_instance().getLatestSideTofData();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Side ToF Frequency: %.1f Hz, Left: %u, Right: %u", frequency, tofData.leftCounts, tofData.rightCounts);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void colorSensorLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    float frequency = SensorDataBuffer::get_instance().getColorSensorFrequency();
    ColorData colorData = SensorDataBuffer::get_instance().getLatestColorData();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Color Sensor Frequency: %.1f Hz, RGB: (%u,%u,%u)", frequency, colorData.redValue, colorData.greenValue,
             colorData.blueValue);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void log_motor_rpm() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 500;

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    auto rpms = SensorDataBuffer::get_instance().getLatestWheelRPMs();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Motor RPM - Left: %.2f || Right: %.2f", rpms.leftWheelRPM, rpms.rightWheelRPM);
    SerialQueueManager::get_instance().queue_message(buffer);

    lastPrintTime = millis();
}

void displayPerformanceLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second

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
