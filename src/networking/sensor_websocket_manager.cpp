#include "sensor_websocket_manager.h"

SensorWebSocketManager::SensorWebSocketManager() {
    _wsConnected = false;
    _lastConnectionAttempt = 0;

    String pip_id = PreferencesManager::get_instance().get_pip_id();
    _wsClient.addHeader("X-Pip-Id", pip_id);

    if (DEFAULT_ENVIRONMENT == "local") {
        return;
    }
    _wsClient.setCACert(ROOT_CA_CERTIFICATE);
}

void SensorWebSocketManager::connect_to_websocket() {
    // Simple connection - no ping/pong handlers needed
    _wsClient.onEvent([this](WebsocketsEvent event, String data) {
        switch (event) {
            case WebsocketsEvent::ConnectionOpened:
                SerialQueueManager::get_instance().queue_message("[WS_SENSOR] Connection opened");
                this->_wsConnected = true;
                break;
            case WebsocketsEvent::ConnectionClosed:
                SerialQueueManager::get_instance().queue_message("[WS_SENSOR] Connection closed");
                this->_wsConnected = false;
                break;
            default:
                break;
        }
    });

    _lastConnectionAttempt = 0; // Force an immediate connection attempt
}

void SensorWebSocketManager::poll_websocket() {
    SensorWebSocketManager& instance = SensorWebSocketManager::get_instance();
    const uint32_t CURRENT_TIME = millis();

    if (CURRENT_TIME - instance._lastPollTime < POLL_INTERVAL) {
        return;
    }

    instance._lastPollTime = CURRENT_TIME;

    if (WiFiClass::status() != WL_CONNECTED) {
        if (instance._wsConnected) {
            SerialQueueManager::get_instance().queue_message("[WS_SENSOR] WiFi lost during session", SerialPriority::HIGH_PRIO);
            instance._wsConnected = false;
        }
        return;
    }

    // Connection management - try to connect if not connected
    if (!instance._wsConnected && (CURRENT_TIME - instance._lastConnectionAttempt >= CONNECTION_INTERVAL)) {
        instance._lastConnectionAttempt = CURRENT_TIME;

        SerialQueueManager::get_instance().queue_message("[WS_SENSOR] Attempting to connect...");

        // Connect to sensor endpoint
        String url = get_ws_server_url();
        url.replace("/esp32", "/ws-sensor"); // Use different endpoint

        if (!instance._wsClient.connect(url)) {
            SerialQueueManager::get_instance().queue_message("[WS_SENSOR] Connection failed");
        } else {
            SerialQueueManager::get_instance().queue_message("[WS_SENSOR] Connected successfully");
            instance._wsConnected = true;
        }
        return;
    }

    // Only poll if connected
    if (!instance._wsConnected) {
        return;
    }

    try {
        instance._wsClient.poll();
    } catch (const std::exception& e) {
        instance._wsConnected = false;
    }
}
