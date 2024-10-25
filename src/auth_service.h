#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "http_client.h"  // Assuming HttpClient is already implemented

class AuthService {
    private:
        HttpClient& httpClient;  // Reference to the HttpClient instance

    public:
        // Constructor
        AuthService(HttpClient& client);

        // Methods for authentication-related actions
        String login(const String& loginInformation);
        String logout();
};

#endif
