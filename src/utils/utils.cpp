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
  
  char logMessage[64];
  snprintf(logMessage, sizeof(logMessage), "Checking for device at address %d...", addr);
  SerialQueueManager::getInstance().queueMessage(logMessage);

  Wire.beginTransmission(addr);
  error = Wire.endTransmission();
  
  if (error == 0) {
    snprintf(logMessage, sizeof(logMessage), "Device found at address %d!", addr);
    SerialQueueManager::getInstance().queueMessage(logMessage);
    return true;
  } else {
    if (error == 4) {
      snprintf(logMessage, sizeof(logMessage), "Unknown error while checking address %d", addr);
      SerialQueueManager::getInstance().queueMessage(logMessage);
    } else {
      snprintf(logMessage, sizeof(logMessage), "No device found at address %d", addr);
      SerialQueueManager::getInstance().queueMessage(logMessage);
    }
    return false;
  }
}

void scanI2C() {
  byte error, address;
  int devicesFound = 0;
  char logMessage[64];
  
  SerialQueueManager::getInstance().queueMessage("Scanning I2C bus...");
  
  for (address = 1; address < 128; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      snprintf(logMessage, sizeof(logMessage), "Device found at address 0x%02X", address);
      SerialQueueManager::getInstance().queueMessage(logMessage);
      devicesFound++;
    }
  }
  
  if (devicesFound == 0) {
    SerialQueueManager::getInstance().queueMessage("No I2C devices found");
  } else {
    snprintf(logMessage, sizeof(logMessage), "Found %d device(s)", devicesFound);
    SerialQueueManager::getInstance().queueMessage(logMessage);
  }
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

const char* routeToString(RouteType route) {
    switch (route) {
        case RouteType::REGISTER: return "/register";
        case RouteType::SENSOR_DATA: return "/sensor-data";
        case RouteType::BYTECODE_STATUS: return "/bytecode-status";
        case RouteType::WIFI_CONNECTION_RESULT: return "/wifi-connection-result";
        case RouteType::PIP_ID: return "/pip-id";
        case RouteType::SAVED_NETWORKS: return "/saved-networks";
        case RouteType::SCAN_RESULT_ITEM: return "/scan-result-item";
        case RouteType::SCAN_COMPLETE: return "/scan-complete";
        case RouteType::SCAN_STARTED: return "/scan-started";
        case RouteType::MOTORS_DISABLED_USB: return "/motors-disabled-usb";
        case RouteType::PROGRAM_PAUSED_USB: return "/program-paused-usb";
        default: return "";
    }
}
