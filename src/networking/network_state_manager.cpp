#include "network_state_mangager.h"

NetworkMode NetworkStateManager::getCurrentMode() {
    if (_isInAddPipMode) {
        return NetworkMode::ADD_PIP_MODE;     // NEW: Return ADD_PIP_MODE when enabled
    } else if (SerialManager::getInstance().isSerialConnected()) {
        return NetworkMode::SERIAL_MODE;
    } else if (WiFi.status() == WL_CONNECTED) {
        return NetworkMode::WIFI_MODE;
    }
    return NetworkMode::NONE;
}

bool NetworkStateManager::shouldStopWiFiOperations() {
    // Don't stop WiFi operations in ADD_PIP_MODE
    return SerialManager::getInstance().isSerialConnected() && !_isInAddPipMode;  // MODIFIED
}

void NetworkStateManager::setAddPipMode(bool enabled) {
    _isInAddPipMode = enabled;
    // SerialQueueManager::getInstance().queueMessage("ADD_PIP_MODE %s\n", enabled ? "ENABLED" : "DISABLED");
}

bool NetworkStateManager::isInAddPipMode() const {
    return _isInAddPipMode;
}
