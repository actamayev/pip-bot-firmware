#include "sensor_loggers.h"

void multizoneTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print every second
    const uint16_t MAX_DISTANCE = 1250; // Maximum distance cutoff in mm, same as in your example

    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    VL53L7CX_ResultsData multizoneTofData = MultizoneTofSensor::getInstance().getTofData();
    
    SerialQueueManager::getInstance().queueMessage("VL53L7CX ToF Sensor Data");
    SerialQueueManager::getInstance().queueMessage("------------------------\n");
    
    // Print separator line for the grid
    for (int i = 0; i < 8; i++) {
        Serial.print(" --------");
    }
    SerialQueueManager::getInstance().queueMessage("-");
    
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
                    // SerialQueueManager::getInstance().queueMessage(" %4d mm", distance);
                }
            } else {
                Serial.print("    X   "); // No target detected
            }
            Serial.print("|");
        }
        // SerialQueueManager::getInstance().queueMessage();
        
        // Print separator line after each row
        for (int i = 0; i < 8; i++) {
            Serial.print(" --------");
        }
        SerialQueueManager::getInstance().queueMessage("-");
    }
    
    // SerialQueueManager::getInstance().queueMessage();
    lastPrintTime = millis();
}

void imuLogger() {
    static unsigned long lastImuPrintTime = 0;
    const unsigned long IMU_PRINT_INTERVAL = 50; // Print every 500ms
    
    if (millis() - lastImuPrintTime < IMU_PRINT_INTERVAL) return;
    EulerAngles eulerAngles = ImuSensor::getInstance().getEulerAngles();
    
    if (!eulerAngles.isValid) {
        SerialQueueManager::getInstance().queueMessage("Failed to get IMU data");
    } else {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Orientation - Yaw: %.1f° Pitch: %.1f° Roll: %.1f°", 
                eulerAngles.yaw, eulerAngles.roll, eulerAngles.pitch);
        SerialQueueManager::getInstance().queueMessage(buffer, SerialPriority::LOW_PRIO);
    }

    lastImuPrintTime = millis();
}

void sideTofsLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 50; // Print every 500ms
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    SideTofCounts tofCounts = SideTofManager::getInstance().getBothSideTofCounts();
    // DisplayScreen::getInstance().showDistanceSensors(tofCounts);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Left TOF: %u counts              || Right TOF: %u counts", 
            tofCounts.leftCounts, tofCounts.rightCounts);
    SerialQueueManager::getInstance().queueMessage(buffer, SerialPriority::LOW_PRIO);
    lastPrintTime = millis();
}

void setupButtonLoggers() {
    Buttons::getInstance().setButton1ClickHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Button 1 clicked!");
    });
    
    Buttons::getInstance().setButton2ClickHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Button 2 clicked!");
    });
    
    Buttons::getInstance().setButton1LongPressHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Button 1 long pressed for " + String(btn.wasPressedFor()) + " ms");
    });
    
    Buttons::getInstance().setButton2LongPressHandler([](Button2& btn) {
        SerialQueueManager::getInstance().queueMessage("Button 2 long pressed for " + String(btn.wasPressedFor()) + " ms");
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
    // SerialQueueManager::getInstance().queueMessage("Left wheel RPM: %.2f\n", rpms.leftWheelRPM);
    // SerialQueueManager::getInstance().queueMessage("Right wheel RPM: %.2f\n", rpms.rightWheelRPM);
    
    // Update last log time
    lastPrintTime = millis();
}
