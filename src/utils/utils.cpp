#include "utils.h"

void quaternion_to_euler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll) {
    // Roll (x-axis rotation)
    float const SINR_COSP = 2 * (qr * qi + qj * qk);
    float const COSR_COSP = 1 - (2 * (qi * qi + qj * qj));
    roll = atan2(SINR_COSP, COSR_COSP);

    // Pitch (y-axis rotation)
    float const SINP = 2 * (qr * qj - qk * qi);
    if (abs(SINP) >= 1) {
        pitch = copysign(M_PI / 2, SINP); // use 90 degrees if out of range
    } else {
        pitch = asin(SINP);
    }

    // Yaw (z-axis rotation)
    float const SINY_COSP = 2 * (qr * qk + qi * qj);
    float const COSY_COSP = 1 - (2 * (qj * qj + qk * qk));
    yaw = atan2(SINY_COSP, COSY_COSP);

    // Convert to degrees
    yaw *= RAD_TO_DEG;
    pitch *= RAD_TO_DEG;
    roll *= RAD_TO_DEG;
}

bool check_address_on_i2c_line(uint8_t addr) {
    byte error = 0;

    char log_message[64];
    snprintf(log_message, sizeof(log_message), "Checking for device at address %d...", addr);
    SerialQueueManager::get_instance().queue_message(log_message);

    Wire.beginTransmission(addr);
    error = Wire.endTransmission();

    if (error == 0) {
        snprintf(log_message, sizeof(log_message), "Device found at address %d!", addr);
        SerialQueueManager::get_instance().queue_message(log_message);
        return true;
    }
    if (error == 4) {
        snprintf(log_message, sizeof(log_message), "Unknown error while checking address %d", addr);
        SerialQueueManager::get_instance().queue_message(log_message);
    } else {
        snprintf(log_message, sizeof(log_message), "No device found at address %d", addr);
        SerialQueueManager::get_instance().queue_message(log_message);
    }
    return false;
}

void scan_i2_c() {
    byte error = 0;
    byte address = 0;
    int devices_found = 0;
    char log_message[64];

    SerialQueueManager::get_instance().queue_message("Scanning I2C bus...");

    for (address = 1; address < 128; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            snprintf(log_message, sizeof(log_message), "Device found at address 0x%02X", address);
            SerialQueueManager::get_instance().queue_message(log_message);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        SerialQueueManager::get_instance().queue_message("No I2C devices found");
    } else {
        snprintf(log_message, sizeof(log_message), "Found %d device(s)", devices_found);
        SerialQueueManager::get_instance().queue_message(log_message);
    }
}

float calculate_circular_mean(const float angles[], uint8_t count) {
    if (count == 0) {
        return 0.0F;
    }

    float sum_sin = 0.0F;
    float sum_cos = 0.0F;

    for (uint8_t i = 0; i < count; i++) {
        // Convert angle to radians for trigonometric functions
        float const ANGLE_RAD = angles[i] * PI / 180.0F;
        sum_sin += sin(ANGLE_RAD);
        sum_cos += cos(ANGLE_RAD);
    }

    // Calculate mean angle in radians and convert back to degrees
    float mean_angle = atan2(sum_sin, sum_cos) * 180.0F / PI;

    return mean_angle;
}

const char* route_to_string_common(ToCommonMessage route) {
    switch (route) {
        case ToCommonMessage::SENSOR_DATA:
            return "/sensor-data";
        case ToCommonMessage::SENSOR_DATA_MZ:
            return "/sensor-data-mz";
        case ToCommonMessage::DINO_SCORE:
            return "/dino-score";
        case ToCommonMessage::PIP_TURNING_OFF:
            return "/pip-turning-off";
        case ToCommonMessage::HEARTBEAT:
            return "/heartbeat";
        default:
            return "";
    }
}

const char* route_to_string_server(ToServerMessage route) {
    switch (route) {
        case ToServerMessage::DEVICE_INITIAL_DATA:
            return "/device-initial-data";
        case ToServerMessage::BATTERY_MONITOR_DATA_FULL:
            return "/battery-monitor-data-full";
        default:
            return "";
    }
}

const char* route_to_string_serial(ToSerialMessage route) {
    switch (route) {
        case ToSerialMessage::BYTECODE_STATUS:
            return "/bytecode-status";
        case ToSerialMessage::WIFI_CONNECTION_RESULT:
            return "/wifi-connection-result";
        case ToSerialMessage::PIP_ID:
            return "/pip-id";
        case ToSerialMessage::SAVED_NETWORKS:
            return "/saved-networks";
        case ToSerialMessage::SCAN_RESULT_ITEM:
            return "/scan-result-item";
        case ToSerialMessage::SCAN_COMPLETE:
            return "/scan-complete";
        case ToSerialMessage::SCAN_STARTED:
            return "/scan-started";
        case ToSerialMessage::MOTORS_DISABLED_USB:
            return "/motors-disabled-usb";
        case ToSerialMessage::PROGRAM_PAUSED_USB:
            return "/program-paused-usb";
        case ToSerialMessage::BATTERY_MONITOR_DATA_ITEM:
            return "/battery-monitor-data-item";
        case ToSerialMessage::BATTERY_MONITOR_DATA_COMPLETE:
            return "/battery-monitor-data-complete";
        case ToSerialMessage::WIFI_DELETED_NETWORK:
            return "/wifi-deleted-network";
        default:
            return "";
    }
}
