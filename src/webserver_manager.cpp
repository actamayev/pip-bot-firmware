#include "webserver_manager.h"
#include "config.h"
#include "wifi_manager.h"

DNSServer dnsServer;
WebServer server(80);

void WebServerManager::startWebServer() {
	IPAddress apIP(192, 168, 4, 1);
	dnsServer.start(DNS_PORT, "*", apIP); // TODO: This re-direct doesn't work (only works from HTTP URLs, not something like https://google.com)

	server.on("/", []() {
		String html = "<html><body><h1>Wi-Fi Configuration</h1>"
						"<form action=\"/connect\" method=\"POST\">"
						"SSID: <input type=\"text\" name=\"ssid\"><br>"
						"Password: <input type=\"text\" name=\"password\"><br>"
						"<input type=\"submit\" value=\"Connect\">"
						"</form></body></html>";
		server.send(200, "text/html", html);
	});

	server.on("/connect", []() {
		String ssid = server.arg("ssid");
		String password = server.arg("password");

		preferences.begin("wifi-creds", false);
		preferences.putString("ssid", ssid);
		preferences.putString("password", password);
		preferences.end();

		bool connectionStatus = wifiManager.connectToStoredWiFi();

       if (connectionStatus) {
            String successHtml = "<html><body><h1>Connected successfully!</h1>"
                                 "<p>ESP will reboot in 2 seconds...</p></body></html>";
            server.send(200, "text/html", successHtml);
            Serial.println("Wi-Fi connected. ESP will reboot in 2 seconds...");
            delay(2000);
            ESP.restart();
        } else {
            String errorHtml = "<html><body><h1>Failed to connect to Wi-Fi</h1>"
                               "<p>Please try again.</p></body></html>";
            server.send(500, "text/html", errorHtml);
            Serial.println("Failed to connect to Wi-Fi. No reboot.");
        }
	});

	server.onNotFound([]() {
		server.send(404, "text/plain", "Not Found");
	});

	server.begin();
}

void WebServerManager::handleClientRequests() {
	if (WiFi.getMode() == WIFI_AP) {
        // Only process DNS requests in AP mode
        dnsServer.processNextRequest();
    }
	server.handleClient();
}

WebServerManager webserverManager;  // Create global instance
