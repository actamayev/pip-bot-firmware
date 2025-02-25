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
    const unsigned long IMU_PRINT_INTERVAL = 500; // Print every 500ms
    
    if (millis() - lastImuPrintTime >= IMU_PRINT_INTERVAL) {
        EulerAngles eulerAngles = Sensors::getInstance().getEulerAngles();
        
        if (!eulerAngles.isValid) {
            Serial.println("Failed to get IMU data");
        } else {
            Serial.print("Orientation - Yaw: ");
            Serial.print(eulerAngles.yaw, 1);
            Serial.print("° Pitch: ");
            Serial.print(eulerAngles.pitch, 1);
            Serial.print("° Roll: ");
            Serial.print(eulerAngles.roll, 1);
            Serial.println("°");
        }
        
        lastImuPrintTime = millis();
    }
}
