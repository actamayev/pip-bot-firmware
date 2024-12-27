#include "./include/config.h"
#include "./include/wifi_manager.h"
#include "./include/webserver_manager.h"

DNSServer dnsServer;
WebServer server(80);

void WebServerManager::startWebServer() {
	IPAddress apIP(192, 168, 4, 1);
	dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/setup", HTTP_GET, []() {
        // Get the full URL
        String fullUrl = server.uri();
        String argument = server.arg("plain"); // Get any POST data if present
        
        // If no POST data, try to get credentials from URL query
        if (argument.isEmpty()) {
            // Iterate through all arguments
            for (int i = 0; i < server.args(); i++) {
                if (server.argName(i) == "credentials") {
                    argument = server.arg(i);
                    break;
                }
            }
        }
        
        // If still no credentials found, return error
        if (argument.isEmpty()) {
            String errorHtml = "<html><body><script>"
                             "alert('No credentials provided');"
                             "window.close();"
                             "</script></body></html>";
            server.send(400, "text/html", errorHtml);
            return;
        }

        // Use mbedtls for base64 decoding
        size_t decodedLength;
        mbedtls_base64_decode(NULL, 0, &decodedLength, 
            (const unsigned char*)argument.c_str(), argument.length());
    
        unsigned char* decodedData = new unsigned char[decodedLength + 1];
        mbedtls_base64_decode(decodedData, decodedLength + 1, &decodedLength, 
            (const unsigned char*)argument.c_str(), argument.length());
        
        String decodedCredentials = String((char*)decodedData);
        delete[] decodedData;

        // Parse JSON
        DynamicJsonDocument doc(200);
        DeserializationError error = deserializeJson(doc, decodedCredentials);
        
        if (error) {
            String errorHtml = "<html><body><script>"
                             "alert('Invalid credentials format');"
                             "window.close();"
                             "</script></body></html>";
            server.send(400, "text/html", errorHtml);
            return;
        }

        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        Serial.printf("Attempting to connect to: %s\n", ssid);

        bool connectionStatus = WiFiManager::getInstance().attemptNewWifiConnection(ssid, password);

        if (connectionStatus) {
            preferences.begin("wifi-creds", false);
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.end();

            String successHtml = "<html><body><script>"
                               "setTimeout(() => {"
                               "  alert('WiFi connected successfully! You can close this tab.');"
                               "  window.close();"
                               "}, 1000);</script>"
                               "<h1>Connected Successfully!</h1>"
                               "<p>This tab will attempt to close automatically...</p>"
                               "</body></html>";
            server.send(200, "text/html", successHtml);
            Serial.println("Wi-Fi connected via setup endpoint");
        } else {
            String errorHtml = "<html><body><script>"
                             "alert('Failed to connect to WiFi. Please try again.');"
                             "window.close();"
                             "</script>"
                             "<h1>Connection Failed</h1>"
                             "<p>Please try again...</p>"
                             "</body></html>";
            server.send(500, "text/html", errorHtml);
            
            preferences.begin("wifi-creds", false);
            preferences.clear();
            preferences.end();
            
            Serial.println("Failed to connect via setup endpoint");
        }
    });

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

		bool connectionStatus = WiFiManager::getInstance().attemptNewWifiConnection(ssid, password);

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

// TODO: Figure out if this is better:
void WebServerManager::handleClientRequests() {
	// Only proceed if WiFi is connected or in Access Point mode
	if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP) return;

	if (WiFi.getMode() == WIFI_AP) {
		// Only process DNS requests in AP mode
		dnsServer.processNextRequest();
	}

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
