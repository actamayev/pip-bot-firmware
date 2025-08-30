#include "sensor_loggers.h"

void imuLogger() {
    static unsigned long lastImuPrintTime = 0;
    const unsigned long IMU_PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    // Print frequency report every second
    if (millis() - lastImuPrintTime < IMU_PRINT_INTERVAL) return;
    
    // Debug: Check if we're getting IMU data at all
    EulerAngles eulerAngles = SensorDataBuffer::getInstance().getLatestEulerAngles();
    float frequency = SensorDataBuffer::getInstance().getImuFrequency();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "IMU Frequency: %.1f Hz, Data Valid: %s, Yaw: %.1f, Pitch: %.1f, Roll: %.1f", 
             frequency, eulerAngles.isValid ? "YES" : "NO", eulerAngles.yaw, eulerAngles.pitch, eulerAngles.roll);
    SerialQueueManager::getInstance().queueMessage(buffer);
    
    lastImuPrintTime = millis();
}

void sideTofsLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 50; // Print every 500ms
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    SideTofData tofCounts = SensorDataBuffer::getInstance().getLatestSideTofData();
    // DisplayScreen::getInstance().showDistanceSensors(tofCounts);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Left TOF: %u counts              || Right TOF: %u counts", 
            tofCounts.leftCounts, tofCounts.rightCounts);
    SerialQueueManager::getInstance().queueMessage(buffer);
    lastPrintTime = millis();
}

void setupButtonLoggers() {
    Buttons::getInstance().setLeftButtonClickHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Left Button clicked!");
    });
    
    Buttons::getInstance().setRightButtonClickHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Right Button clicked!");
    });
    
    Buttons::getInstance().setLeftButtonLongPressHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Left Button long pressed for " + String(btn.wasPressedFor()) + " ms");
    });
    
    Buttons::getInstance().setRightButtonLongPressHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Right Button long pressed for " + String(btn.wasPressedFor()) + " ms");
    });
}

void multizoneTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    
    float frequency = SensorDataBuffer::getInstance().getMultizoneTofFrequency();
    TofData tofData = SensorDataBuffer::getInstance().getLatestTofData();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Multizone ToF Frequency: %.1f Hz, Data Valid: %s, Object Detected: %s", 
             frequency, tofData.isValid ? "YES" : "NO", tofData.isObjectDetected ? "Object Detected" : "Object not detected");
    SerialQueueManager::getInstance().queueMessage(buffer);
    
    lastPrintTime = millis();
}

void sideTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    
    float frequency = SensorDataBuffer::getInstance().getSideTofFrequency();
    SideTofData tofData = SensorDataBuffer::getInstance().getLatestSideTofData();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Side ToF Frequency: %.1f Hz, Left: %u, Right: %u", 
             frequency, tofData.leftCounts, tofData.rightCounts);
    SerialQueueManager::getInstance().queueMessage(buffer);
    
    lastPrintTime = millis();
}

void colorSensorLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    
    float frequency = SensorDataBuffer::getInstance().getColorSensorFrequency();
    ColorData colorData = SensorDataBuffer::getInstance().getLatestColorData();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Color Sensor Frequency: %.1f Hz, RGB: (%u,%u,%u)", 
             frequency, colorData.redValue, colorData.greenValue, colorData.blueValue);
    SerialQueueManager::getInstance().queueMessage(buffer);
    
    lastPrintTime = millis();
}

void log_motor_rpm() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 500;
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    auto rpms = SensorDataBuffer::getInstance().getLatestWheelRPMs();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Motor RPM - Left: %.2f || Right: %.2f", 
             rpms.leftWheelRPM, rpms.rightWheelRPM);
    SerialQueueManager::getInstance().queueMessage(buffer);
    
    lastPrintTime = millis();
}

void irSensorLogger() {
    static unsigned long lastPrintTime = 0;
    static unsigned long readingCount = 0;
    static unsigned long lastResetTime = millis();
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    // Count this as a reading attempt
    readingCount++;
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    
    // Calculate frequency
    unsigned long timeElapsed = millis() - lastResetTime;
    float readingsPerSecond = (float)readingCount * 1000.0 / (float)timeElapsed;
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "IR_SENSOR: %.2f Hz (%lu readings)", 
             readingsPerSecond, readingCount);
    SerialQueueManager::getInstance().queueMessage(buffer);
    
    // Reset counters for next measurement period
    readingCount = 0;
    lastResetTime = millis();
    lastPrintTime = millis();
}

void displayPerformanceLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    
    DisplayScreen& display = DisplayScreen::getInstance();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "DISPLAY: %.2f Hz actual I2C (Generated: %lu, Updates: %lu, Skipped: %lu)",
             display.getDisplayUpdateRate(),
             display.getContentGenerationCount(),
             display.getDisplayUpdateCount(),
             display.getSkippedUpdateCount());
    SerialQueueManager::getInstance().queueMessage(buffer);
    
    // Reset performance counters for next measurement period
    display.resetPerformanceCounters();
    lastPrintTime = millis();
}
