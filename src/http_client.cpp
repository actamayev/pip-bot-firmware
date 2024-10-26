#include <WiFiClientSecure.h>
#include "config.h"
#include "http_client.h"

HttpClient::HttpClient() {
    client.setCACert(rootCACertificate);
}

String HttpClient::getPathHeaderString(PathHeader pathHeader) {
    switch (pathHeader) {
        case PathHeader::Auth: return "/auth";
        default: return "";
    }
}

String HttpClient::getPathFooterString(PathFooter pathFooter) {
    switch (pathFooter) {
        case PathFooter::Login: return "/login";
        case PathFooter::Logout: return "/logout";
        default: return "";
    }
}

const char* HttpClient::generateFullPath(PathHeader pathHeader, PathFooter pathFooter) {
    return (getPathHeaderString(pathHeader) + getPathFooterString(pathFooter)).c_str();
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

    client.print("GET " + String(endpoint) + " HTTP/1.1\r\n");
    client.print("Host: " + String(server_url) + "\r\n");
    client.print("Connection: close\r\n\r\n");

    String response = "";
    while (client.connected()) {
        response += client.readString();
    }

    client.stop(); // Close connection
    return response;
}
