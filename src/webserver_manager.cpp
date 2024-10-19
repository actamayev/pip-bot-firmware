#include "webserver_manager.h"
#include "config.h"
#include "wifi_manager.h"

DNSServer dnsServer;
WebServer server(80);

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

    Serial.println("In handle connect: receied ssid: " + ssid);
    Serial.println("In handle connect: recevied pw: " + password);
    
    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    
    wifiManager.connectToStoredWiFi();
    server.send(200, "text/html", "Connecting... ESP will reboot.");
    delay(2000);
    ESP.restart();
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });

  server.begin();
}

void WebServerManager::handleClientRequests() {
  dnsServer.processNextRequest();
  server.handleClient();
}

WebServerManager webserverManager;  // Create global instance
