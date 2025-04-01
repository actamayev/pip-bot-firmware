#include "./sensor_loggers.h"

void multizoneTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print every second

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    auto& sensors = Sensors::getInstance();
    VL53L5CX_ResultsData multizoneTofData = sensors.getMultizoneTofData();
    for (int y = 0; y <= TOF_IMAGE_RESOLUTION * (TOF_IMAGE_RESOLUTION - 1); y += TOF_IMAGE_RESOLUTION) {
        // Print left sensor data
        for (int x = TOF_IMAGE_RESOLUTION - 1; x >= 0; x--) {
            Serial.print("\t");
            Serial.print(multizoneTofData.distance_mm[x + y]);
        }   
        Serial.println();
    }
    Serial.println();
    lastPrintTime = millis();
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
    const unsigned long PRINT_INTERVAL = 500; // Print every 500ms
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    SideTofDistances tofDistances = Sensors::getInstance().getBothSideTofDistances();
    DisplayScreen::getInstance().showDistanceSensors(tofDistances);

    // Print side by side with alignment
    // Serial.print("Left TOF: ");
    // Serial.print(tofDistances.leftDistance);
    // Serial.print(" mm              || Right TOF: ");
    // Serial.print(tofDistances.rightDistance);
    // Serial.println(" mm");

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
