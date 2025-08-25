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
    const unsigned long IMU_PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    // Print frequency report every second
    if (millis() - lastImuPrintTime < IMU_PRINT_INTERVAL) return;
    
    // Debug: Check if we're getting IMU data at all
    EulerAngles eulerAngles = SensorDataBuffer::getInstance().getLatestEulerAngles();
    float frequency = SensorDataBuffer::getInstance().getImuFrequency();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "IMU Frequency: %.1f Hz, Data Valid: %s, Yaw: %.1f", 
             frequency, eulerAngles.isValid ? "YES" : "NO", eulerAngles.yaw);
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

void multizoneTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print frequency every 1 second
    
    if (millis() - lastPrintTime < PRINT_INTERVAL) return;
    
    float frequency = SensorDataBuffer::getInstance().getMultizoneTofFrequency();
    TofData tofData = SensorDataBuffer::getInstance().getLatestTofData();
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Multizone ToF Frequency: %.1f Hz, Data Valid: %s", 
             frequency, tofData.isValid ? "YES" : "NO");
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
