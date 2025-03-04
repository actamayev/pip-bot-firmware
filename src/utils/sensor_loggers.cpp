#include "../sensors/sensors.h"
#include "./utils.h"

void multizoneTofLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 1000; // Print every second

    if (millis() - lastPrintTime >= PRINT_INTERVAL) {
        auto& sensors = Sensors::getInstance();
        // Option 1: If getMultizoneTofData() returns a struct (not a pointer)
        VL53L5CX_ResultsData multizoneTofData = sensors.getMultizoneTofData();
        for (int y = 0; y <= TOF_IMAGE_RESOLUTION * (TOF_IMAGE_RESOLUTION - 1); y += TOF_IMAGE_RESOLUTION) {
            // Print left sensor data
            for (int x = TOF_IMAGE_RESOLUTION - 1; x >= 0; x--) {
                Serial.print("\t");
                Serial.print(multizoneTofData.distance_mm[x + y]);
            }
            
            Serial.print("\t|\t"); // Separator between left and right data
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

void sideTofsLogger() {
    static unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 500; // Print every 500ms
    
    if (millis() - lastPrintTime >= PRINT_INTERVAL) {
        uint16_t leftTofDistance = Sensors::getInstance().getLeftSideTofDistance();
        uint16_t rightTofDistance = Sensors::getInstance().getRightSideTofDistance();

        // Print side by side with alignment
        Serial.print("Left TOF: ");
        Serial.print(leftTofDistance);
        Serial.print(" mm              || Right TOF: ");
        Serial.print(rightTofDistance);
        Serial.println(" mm");
        
        lastPrintTime = millis();
    }
}
