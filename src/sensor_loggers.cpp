#include "./include/sensors.h"
#include "./include/utils.h"

void tofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print every second

    if (millis() - lastPrintTime >= PRINT_INTERVAL) {
        auto& sensors = Sensors::getInstance();
        const VL53L5CX_ResultsData* leftData;
        const VL53L5CX_ResultsData* rightData;

        if (sensors.getTofData(&leftData, &rightData)) {
            // Print in 8x8 grid format
            const int imageWidth = 8;
            Serial.println("\nDistance readings (mm):");
            Serial.println("Left Sensor\t\t\t\t\t\t\t\t|\tRight Sensor");
            Serial.println("--------------------------------------------------------------------------------");

            for (int y = 0; y <= imageWidth * (imageWidth - 1); y += imageWidth) {
                // Print left sensor data
                for (int x = imageWidth - 1; x >= 0; x--) {
                    Serial.print("\t");
                    Serial.print(leftData->distance_mm[x + y]);
                }
                
                Serial.print("\t|\t"); // Separator between left and right data
                
                // Print right sensor data
                for (int x = imageWidth - 1; x >= 0; x--) {
                    Serial.print("\t");
                    Serial.print(rightData->distance_mm[x + y]);
                }
                Serial.println();
            }
        } else {
            Serial.println("Failed to get TOF data");
        }

        lastPrintTime = millis();
    }
}

void imuLogger() {
    static unsigned long lastImuPrintTime = 0;
    const unsigned long IMU_PRINT_INTERVAL = 50; // Print every 50ms (20Hz)
    
    if (millis() - lastImuPrintTime >= IMU_PRINT_INTERVAL) {
        auto& sensors = Sensors::getInstance();
        float qX, qY, qZ, qW;
        
        if (sensors.getQuaternion(qX, qY, qZ, qW)) {
            float yaw, pitch, roll;
            quaternionToEuler(qW, qX, qY, qZ, yaw, pitch, roll);
            
            Serial.print("Orientation - Yaw: ");
            Serial.print(yaw, 1);
            Serial.print("° Pitch: ");
            Serial.print(pitch, 1);
            Serial.print("° Roll: ");
            Serial.print(roll, 1);
            Serial.println("°");
        } else {
            Serial.println("Failed to get IMU data");
        }
        
        lastImuPrintTime = millis();
    }
}

void irLogger() {
    static unsigned long lastIrSend = 0;
    const unsigned long IR_SEND_INTERVAL = 1000;  // Send every second

    // Send periodic IR signal
    if (millis() - lastIrSend >= IR_SEND_INTERVAL) {
        Serial.println("Sending IR command: 51");
        Sensors::getInstance().sendIrCommand(51);
        lastIrSend = millis();
    }

    // Check for received IR signals
    if (Sensors::getInstance().getIrData()) {
        Serial.println("✓ IR signal received!");
    }
}
