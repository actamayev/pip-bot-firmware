#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <WiFiClientSecure.h>

class HttpClient {
    public:
        // Constructor that takes the server URL and CA certificate
        HttpClient();

        String post(const char* endpoint, const String& data);

        String get(const char* endpoint);
    
    private:
        WiFiClientSecure client;    // Secure WiFi client
};

#endif
