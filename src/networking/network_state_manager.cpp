#include "./network_state_mangager.h"

NetworkMode NetworkStateManager::getCurrentMode() {
    if (SerialManager::getInstance().isConnected) {
        return NetworkMode::SERIAL_MODE;
    } else if (WiFi.status() == WL_CONNECTED) {
        return NetworkMode::WIFI_MODE;
    }
    return NetworkMode::NONE;
}

bool NetworkStateManager::shouldStopWiFiOperations() {
    return SerialManager::getInstance().isConnected;
}
