#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <WiFiClientSecure.h>

class HttpClient {
    private:
        WiFiClientSecure client;    // Secure WiFi client
    public:
        // Constructor that takes the server URL and CA certificate
        HttpClient(const char* ca_cert);

        // Method for making POST requests
        String post(const char* endpoint, const String& data);

        String get(const char* endpoint);
};

#endif
