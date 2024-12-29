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
				"<html><head><meta name='viewport' content='width=device-width,initial-scale=1'></head>"
				"<body>"
				"<div style='text-align:center; padding: 20px;'>"
				"<h2>WiFi Connected Successfully!</h2>"
				"<p>You can close this window and return to the application.</p>"
				"</div>"
				"<script>"
				"alert('WiFi connected successfully!');"
				"setTimeout(() => { window.close(); }, 1000);"
				"</script></body></html>"
			);
			Serial.println("Wi-Fi connected via setup endpoint");
			WiFi.mode(WIFI_STA);
			WebSocketManager::getInstance().connectToWebSocket();
		} else {
			// TODO: If the connection wasn't successful, it should display a form on the ESP to enter credentials
			preferences.begin("wifi-creds", false);
			preferences.clear();
			preferences.end();
			
			server.send(200, "text/html", 
				"<html><head><meta name='viewport' content='width=device-width,initial-scale=1'></head>"
				"<body>"
				"<div style='text-align:center; padding: 20px;'>"
				"<h2>Failed to Connect</h2>"
				"<p>Could not connect to the WiFi network. Please try again.</p>"
				"</div>"
				"<script>"
				"alert('Failed to connect to WiFi');"
				"setTimeout(() => { window.close(); }, 1000);"
				"</script></body></html>"
			);
			Serial.println("Failed to connect via setup endpoint");
			WiFiManager::getInstance().startAccessPoint();
		}
	});

	server.onNotFound([]() {
		server.send(404, "text/plain", "Not Found");
	});

	server.begin();
}

// TODO: Figure out if this is better:
void WebServerManager::handleClientRequests() {
	// TODO: Test with only server.handleClient();
	// Only proceed if WiFi is connected or in Access Point mode
	// if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP) return;

	// if (WiFi.getMode() == WIFI_AP) {
	// 	// Only process DNS requests in AP mode
	// 	dnsServer.processNextRequest();
	// }

	// Handle HTTP server requests in both Station and AP modes
	server.handleClient();
}

// void WebServerManager::handleClientRequests() {
// 	// TODO: Figure out if this should be added?
// 	// if (WiFi.status() != WL_CONNECTED) return;
// 	if (WiFi.getMode() == WIFI_AP) {
//         // Only process DNS requests in AP mode
//         dnsServer.processNextRequest();
//     }
// 	server.handleClient();
// }
