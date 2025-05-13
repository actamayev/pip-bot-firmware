#include "sensor_loggers.h"

void multizoneTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print every second
    const uint16_t MAX_DISTANCE = 1250; // Maximum distance cutoff in mm, same as in your example

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    VL53L7CX_ResultsData multizoneTofData = MultizoneTofSensor::getInstance().getTofData();
    
    Serial.println("VL53L7CX ToF Sensor Data");
    Serial.println("------------------------\n");
    
    // Print separator line for the grid
    for (int i = 0; i < 8; i++) {
        Serial.print(" --------");
    }
    Serial.println("-");
    
    // Print the grid (traversing rows from bottom to top to match orientation)
    for (int row = 8 - 1; row >= 0; row--) {
        Serial.print("|");
        
        // Traverse columns from right to left to match orientation
        for (int col = 8 - 1; col >= 0; col--) {
            // Calculate proper index in the data array
            int index = row * 8 + col;
            
            // Check if a target was detected at this position
            if (multizoneTofData.nb_target_detected[index] > 0) {
                // Apply the distance cutoff
                uint16_t distance = multizoneTofData.distance_mm[index];
                if (distance > MAX_DISTANCE) {
                    Serial.print("    X   ");
                } else {
                    Serial.printf(" %4d mm", distance);
                }
            } else {
                Serial.print("    X   "); // No target detected
            }
            Serial.print("|");
        }
        Serial.println();
        
        // Print separator line after each row
        for (int i = 0; i < 8; i++) {
            Serial.print(" --------");
        }
        Serial.println("-");
    }
    
    Serial.println();
    lastPrintTime = millis();
}

void imuLogger() {
    static unsigned long lastImuPrintTime = 0;
    const unsigned long IMU_PRINT_INTERVAL = 500; // Print every 500ms
    
    if (millis() - lastImuPrintTime < IMU_PRINT_INTERVAL) return;
    EulerAngles eulerAngles = ImuSensor::getInstance().getEulerAngles();
    
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
    SideTofCounts tofCounts = SideTofManager::getInstance().getBothSideTofCounts();
    // DisplayScreen::getInstance().showDistanceSensors(tofCounts);

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
