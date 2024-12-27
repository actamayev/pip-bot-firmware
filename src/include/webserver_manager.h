#pragma once

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "./singleton.h"

class WebServerManager : public Singleton<WebServerManager> {
    friend class Singleton<WebServerManager>;

    public:
        WebServerManager() = default;
        void startWebServer();
        void handleClientRequests();
};
