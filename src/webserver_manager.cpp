#include "./include/utils.h"
#include "./include/config.h"
#include "./include/wifi_manager.h"
#include "./include/webserver_manager.h"
#include "./include/websocket_manager.h"

DNSServer dnsServer;
WebServer server(80);

void WebServerManager::startWebServer() {
	IPAddress apIP(192, 168, 4, 1);
	dnsServer.start(DNS_PORT, "*", apIP);

	server.on("/setup", HTTP_GET, []() {
		String encodedCredentials = "";
		WiFiCredentials wifiCredentials;

		bool extractCredentialsResult = extractCredentials(encodedCredentials, server, wifiCredentials);

		if (extractCredentialsResult == false) {
			server.send(200, "text/html", 
				"<html><head><meta name='viewport' content='width=device-width,initial-scale=1'></head>"
				"<body><script>"
				"alert('Invalid credentials format');"
				"window.close();"
				"</script></body></html>"
			);
			return;
		}

		bool connectionStatus = WiFiManager::getInstance().attemptNewWifiConnection(wifiCredentials);

		if (connectionStatus) {
			preferences.begin("wifi-creds", false);
			preferences.putString("ssid", wifiCredentials.ssid);
			preferences.putString("password", wifiCredentials.password);
			preferences.end();

			server.send(200, "text/html", 
				"<html><script>window.close();</script></html>"
			);
			Serial.println("Wi-Fi connected via setup endpoint");
			delay(1000);
			WiFi.softAPdisconnect(true); // kicks connected users off the Pip's AP
			WebSocketManager::getInstance().connectToWebSocket();
		} else {
			preferences.begin("wifi-creds", false);
			preferences.clear();
			preferences.end();

			server.send(200, "text/html", 
				"<html><script>window.close();</script></html>"
			);
			Serial.println("Failed to connect via setup endpoint");
			delay(1000);
			WiFi.softAPdisconnect(true); // kicks connected users off the Pip's AP
			WiFiManager::getInstance().startAccessPoint();
		}
	});

	server.onNotFound([]() {
		server.send(404, "text/plain", "Not Found");
	});

	server.begin();
}

void WebServerManager::handleClientRequests() {
	// Handle HTTP server requests in both Station and AP modes
	server.handleClient();
}
