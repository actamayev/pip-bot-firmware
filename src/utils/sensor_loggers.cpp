#include "sensor_loggers.h"

// void multizoneTofLogger() {
//     static unsigned long lastPrintTime = 0;
//     const unsigned long PRINT_INTERVAL = 50;
//     const uint16_t MAX_DISTANCE = 1250;

//     if (millis() - lastPrintTime < PRINT_INTERVAL) return;

//     VL53L7CX_ResultsData multizoneTofData = MultizoneTofSensor::getInstance().getTofData();
    
//     // Send header as separate messages
//     SerialQueueManager::getInstance().queueMessage("VL53L7CX ToF Sensor Data", SerialPriority::LOW_PRIO);
//     SerialQueueManager::getInstance().queueMessage("------------------------", SerialPriority::LOW_PRIO);
    
//     // Build and send each row pair (separator + data) as we go
//     for (int row = 7; row >= 0; row--) {
//         // Build separator line
//         char separatorLine[80];  // Small stack buffer
//         int pos = 0;
//         for (int i = 0; i < 8; i++) {
//             pos += snprintf(separatorLine + pos, sizeof(separatorLine) - pos, " --------");
//         }
//         snprintf(separatorLine + pos, sizeof(separatorLine) - pos, "-");
//         SerialQueueManager::getInstance().queueMessage(separatorLine, SerialPriority::LOW_PRIO);
        
//         // Build data row
//         char dataRow[80];  // Small stack buffer
//         pos = 0;
//         pos += snprintf(dataRow + pos, sizeof(dataRow) - pos, "|");
        
//         for (int col = 7; col >= 0; col--) {
//             int index = row * 8 + col;
            
//             if (multizoneTofData.nb_target_detected[index] > 0) {
//                 uint16_t distance = multizoneTofData.distance_mm[index];
//                 if (distance > MAX_DISTANCE) {
//                     pos += snprintf(dataRow + pos, sizeof(dataRow) - pos, "    X   |");
//                 } else {
//                     pos += snprintf(dataRow + pos, sizeof(dataRow) - pos, " %4d mm|", distance);
//                 }
//             } else {
//                 pos += snprintf(dataRow + pos, sizeof(dataRow) - pos, "    X   |");
//             }
//         }
//         SerialQueueManager::getInstance().queueMessage(dataRow, SerialPriority::LOW_PRIO);
//     }
    
//     // Send final separator
//     char finalSeparator[80];
//     int pos = 0;
//     for (int i = 0; i < 8; i++) {
//         pos += snprintf(finalSeparator + pos, sizeof(finalSeparator) - pos, " --------");
//     }
//     snprintf(finalSeparator + pos, sizeof(finalSeparator) - pos, "-");
//     SerialQueueManager::getInstance().queueMessage(finalSeparator, SerialPriority::LOW_PRIO);
    
//     lastPrintTime = millis();
// }

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

// void sideTofsLogger() {
//     static unsigned long lastPrintTime = 0;
//     const unsigned long PRINT_INTERVAL = 50; // Print every 500ms
    
//     if (millis() - lastPrintTime < PRINT_INTERVAL) return;
//     SideTofCounts tofCounts = SideTofManager::getInstance().getBothSideTofCounts();
//     // DisplayScreen::getInstance().showDistanceSensors(tofCounts);

//     char buffer[128];
//     snprintf(buffer, sizeof(buffer), "Left TOF: %u counts              || Right TOF: %u counts", 
//             tofCounts.leftCounts, tofCounts.rightCounts);
//     SerialQueueManager::getInstance().queueMessage(buffer, SerialPriority::LOW_PRIO);
//     lastPrintTime = millis();
// }

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
    const unsigned long PRINT_INTERVAL = 500;
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;

    auto rpms = encoderManager.getBothWheelRPMs();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Motor RPM - Left: %.2f || Right: %.2f", 
             rpms.leftWheelRPM, rpms.rightWheelRPM);
    SerialQueueManager::getInstance().queueMessage(buffer, SerialPriority::LOW_PRIO);
    
    lastPrintTime = millis();
}
