#include <WiFiClientSecure.h>
#include "config.h"
#include "http_client.h"

// Constructor implementatioan
HttpClient::HttpClient() {
    // Set the CA certificate for the secure connection
    client.setCACert(rootCACertificate);
}

String HttpClient::post(const char* endpoint, const String& data) {
    if (!client.connect(server_url, 443)) {
        Serial.println("HTTPS connection failed.");
        return "";
    }

    client.print("POST " + String(endpoint) + " HTTP/1.1\r\n");
    client.print("Host: " + String(server_url) + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(data.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(data);

    // Capture response
    String response = "";
    while (client.connected()) {
        response += client.readString();
    }

    client.stop(); // Close connection
    return response;
}

String HttpClient::get(const char* endpoint) {
    if (!client.connect(server_url, 443)) {
        Serial.println("HTTPS connection failed.");
        return "";
    }

    // Send GET request
    client.print("GET " + String(endpoint) + " HTTP/1.1\r\n");
    client.print("Host: " + String(server_url) + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Capture response
    String response = "";
    while (client.connected()) {
        response += client.readString();
    }

    client.stop(); // Close connection
    return response;
}
