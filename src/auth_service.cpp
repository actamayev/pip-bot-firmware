#include "auth_service.h"

// Constructor implementation
AuthService::AuthService(HttpClient& client) : httpClient(client) {}

// Login method implementation
String AuthService::login(const String& loginInformation) {
    // Send a POST request to the /auth/login endpoint
    return httpClient.post("/auth/login", loginInformation);
}

// Logout method implementation
String AuthService::logout() {
    // Send a POST request to the /auth/logout endpoint
    return httpClient.post("/auth/logout", "{}");  // Empty JSON object for logout
}
