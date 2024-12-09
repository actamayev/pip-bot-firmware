#pragma once

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

class WebServerManager {
    public:
        static WebServerManager& getInstance() {
            if (instance == nullptr) {
                instance = new WebServerManager();
            }
            return *instance;
        }
        void startWebServer();
        void handleClientRequests();
    private:
        static WebServerManager* instance;

        WebServerManager();

        WebServerManager(const WebServerManager&) = delete;
        WebServerManager& operator=(const WebServerManager&) = delete;
};
