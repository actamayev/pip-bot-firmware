#include "utils.h"

void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll) {
    // Roll (x-axis rotation)
    float sinr_cosp = 2 * (qr * qi + qj * qk);
    float cosr_cosp = 1 - 2 * (qi * qi + qj * qj);
    roll = atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2 * (qr * qj - qk * qi);
    if (abs(sinp) >= 1) pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else pitch = asin(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2 * (qr * qk + qi * qj);
    float cosy_cosp = 1 - 2 * (qj * qj + qk * qk);
    yaw = atan2(siny_cosp, cosy_cosp);
    
    // Convert to degrees
    yaw *= RAD_TO_DEG;
    pitch *= RAD_TO_DEG;
    roll *= RAD_TO_DEG;
}

bool check_address_on_i2c_line(uint8_t addr) {
  byte error;
  
  // SerialQueueManager::getInstance().queueMessage("Checking for device at address %d...", addr);

  Wire.beginTransmission(addr);
  error = Wire.endTransmission();
  
  if (error == 0) {
    // SerialQueueManager::getInstance().queueMessage("Device found at address %d!", addr);
    return true;
  } else {
    if (error == 4) {
      // SerialQueueManager::getInstance().queueMessage("Unknown error while checking address %d", addr);
    } else {
      // SerialQueueManager::getInstance().queueMessage("No device found at address %d", addr);
    }
    return false;
  }
}

void scanI2C() {
  byte error, address;
  int devicesFound = 0;
  
  SerialQueueManager::getInstance().queueMessage("Scanning I2C bus...");
  
  for (address = 1; address < 128; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) Serial.print("0");
      // SerialQueueManager::getInstance().queueMessage(address, HEX);
      devicesFound++;
    }
  }
  
  if (devicesFound == 0) {
    SerialQueueManager::getInstance().queueMessage("No I2C devices found");
  } else {
    // SerialQueueManager::getInstance().queueMessage("Found %d device(s)\n", devicesFound);
  }
  // SerialQueueManager::getInstance().queueMessage();
}

float calculateCircularMean(const float angles[], uint8_t count) {
    if (count == 0) return 0.0f;
    
    float sumSin = 0.0f;
    float sumCos = 0.0f;
    
    for (uint8_t i = 0; i < count; i++) {
        // Convert angle to radians for trigonometric functions
        float angleRad = angles[i] * PI / 180.0f;
        sumSin += sin(angleRad);
        sumCos += cos(angleRad);
    }
    
    // Calculate mean angle in radians and convert back to degrees
    float meanAngle = atan2(sumSin, sumCos) * 180.0f / PI;
    
    return meanAngle;
}
