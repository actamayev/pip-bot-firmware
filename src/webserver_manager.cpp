#include <WiFi.h>
#include "config.h"
#include "wifi_manager.h"
#include "webserver_manager.h"

DNSServer dnsServer;
WebServer server(80);
WebServerManager webServerManager;  // Create global instance

void WebServerManager::startWebServer() {
	IPAddress apIP(192, 168, 4, 1);
	dnsServer.start(DNS_PORT, "*", apIP);

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

		bool connectionStatus = wifiManager.attemptNewWifiConnection(ssid, password);

		if (connectionStatus) {
			String successHtml =
				"<html><body><h1>Connected successfully!</h1>"
				"<p>Please close this tab and re-connect your computer to your previously connected wifi</p></body></html>";

			preferences.begin("wifi-creds", false);
			preferences.putString("ssid", ssid);
			preferences.putString("password", password);
			preferences.end();
			server.send(200, "text/html", successHtml);
			Serial.println("Wi-Fi connected.");
		} else {
			String errorHtml = "<html><body><h1>Failed to connect to Wi-Fi</h1>"
								"<p>Please try again.</p></body></html>";
			server.send(500, "text/html", errorHtml);
			
			// Clear the entered Wifi ssid/password
			preferences.begin("wifi-creds", false);  
			preferences.clear();  // Clear all WiFi credentials from "wifi-creds" namespace
			preferences.end();    // Close the preferences to free up resources

			Serial.println("Failed to connect to Wi-Fi. No reboot.");
		}
	});

	server.onNotFound([]() {
		server.send(404, "text/plain", "Not Found");
	});

	server.begin();
}

void WebServerManager::handleClientRequests() {
	// TODO: Figure out if this should be added?
	// if (WiFi.status() != WL_CONNECTED) {
	// 	return;
	// }
	if (WiFi.getMode() == WIFI_AP) {
        // Only process DNS requests in AP mode
        dnsServer.processNextRequest();
    }
	server.handleClient();
}
