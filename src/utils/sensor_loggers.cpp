#include "./sensor_loggers.h"

void multizoneTofLogger() {
    static unsigned long lastLogTime = 0;
    const unsigned long LOG_INTERVAL = 100; // 100ms to match working script
    
    // Only proceed if it's time to log
    unsigned long currentTime = millis();
    if (currentTime - lastLogTime < LOG_INTERVAL) return;
    
    // Get sensor data
    auto& sensors = Sensors::getInstance();
    VL53L7CX_ResultsData multizoneTofData = sensors.getMultizoneTofData();
    float avgDistance = sensors.getAverageDistanceCenterline();
    bool objectDetected = sensors.isObjectDetected();
    
    // Log the grid using our LogManager - only if queue isn't too full
    if (!LogManager::getInstance().isQueueNearlyFull()) {
        LogManager::getInstance().logTofGrid(&multizoneTofData, avgDistance, objectDetected);
    }
    
    lastLogTime = currentTime;
}

void imuLogger() {
    static unsigned long lastImuPrintTime = 0;
    const unsigned long IMU_PRINT_INTERVAL = 500; // Print every 500ms
    
    if (millis() - lastImuPrintTime < IMU_PRINT_INTERVAL) return;
    EulerAngles eulerAngles = Sensors::getInstance().getEulerAngles();
    
    if (!eulerAngles.isValid) {
        Serial.println("Failed to get IMU data");
    } else {
        // Serial.print("Orientation - Yaw: ");
        // Serial.print(eulerAngles.yaw, 1);
        Serial.print("° Pitch: ");
        Serial.print(eulerAngles.pitch, 1);
        // Serial.print("° Roll: ");
        // Serial.print(eulerAngles.roll, 1);
        Serial.println("°");
    }

    lastImuPrintTime = millis();
}

void sideTofsLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 50; // Print every 500ms
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    SideTofCounts tofCounts = Sensors::getInstance().getBothSideTofCounts();
    // DisplayScreen::getInstance().showDistanceSensors(tofDistances);

    // Print side by side with alignment
    Serial.print("Left TOF: ");
    Serial.print(tofCounts.leftCounts);
    Serial.print(" counts              || Right TOF: ");
    Serial.print(tofCounts.rightCounts);
    Serial.println(" counts");

    lastPrintTime = millis();
}

void setupButtonLoggers() {
    Buttons::getInstance().setButton1ClickHandler([](Button2& btn) {
        Serial.println("Button 1 clicked!");
    });
    
    Buttons::getInstance().setButton2ClickHandler([](Button2& btn) {
        Serial.println("Button 2 clicked!");
    });
    
    Buttons::getInstance().setButton1LongPressHandler([](Button2& btn) {
        Serial.println("Button 1 long pressed for " + String(btn.wasPressedFor()) + " ms");
    });
    
    Buttons::getInstance().setButton2LongPressHandler([](Button2& btn) {
        Serial.println("Button 2 long pressed for " + String(btn.wasPressedFor()) + " ms");
    });
}

void log_motor_rpm() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 500; // Print every 500ms
    // Check if 10ms has elapsed since last log
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    // Get latest RPM values
    auto rpms = encoderManager.getBothWheelRPMs();

    // Log the values
    Serial.printf("Left wheel RPM: %.2f\n", rpms.leftWheelRPM);
    Serial.printf("Right wheel RPM: %.2f\n", rpms.rightWheelRPM);
    
    // Update last log time
    lastPrintTime = millis();
}
