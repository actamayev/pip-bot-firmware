#include "./include/utils.h"

void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll) {
    // Roll (x-axis rotation)
    float sinr_cosp = 2 * (qr * qi + qj * qk);
    float cosr_cosp = 1 - 2 * (qi * qi + qj * qj);
    roll = atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2 * (qr * qj - qk * qi);
    if (abs(sinp) >= 1)
        pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else
        pitch = asin(sinp);

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
  
  Serial.printf("Checking for device at address %d...", addr);

  Wire.beginTransmission(addr);
  error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.printf("Device found at address %d!", addr);
    return true;
  } else {
    if (error == 4) {
      Serial.printf("Unknown error while checking address %d", addr);
    } else {
      Serial.printf("No device found at address %d", addr);
    }
    return false;
  }
}
