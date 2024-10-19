#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <DNSServer.h>
#include <WebServer.h>

class WebServerManager {
public:
	void startWebServer();
	void handleClientRequests();
};

extern WebServerManager webserverManager;

#endif
