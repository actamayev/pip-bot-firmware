#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "http_client.h"
#include "config.h"

class AuthService {
    public:
        AuthService(HttpClient& client);

        String login(const String& loginInformation);
        String logout();
    private:
        HttpClient& httpClient;
};

#endif

