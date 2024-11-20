#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "./firmware_updater.h"

using namespace websockets;

class WebSocketManager {
	private:
        static const size_t JSON_DOC_SIZE = 8 * 1024;      // 8KB for JSON

        websockets::WebsocketsClient wsClient;
        FirmwareUpdater updater;

        void handleMessage(websockets::WebsocketsMessage message);
	public:
		WebSocketManager();
		void connectToWebSocket();
		void pollWebSocket();
};

#endif
