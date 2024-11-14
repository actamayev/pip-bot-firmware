#include "./include/auth_service.h"
#include "./include/config.h"

AuthService::AuthService(HttpClient& client) : httpClient(client) {}

String AuthService::login(const String& loginInformation) {
    return httpClient.post(
        httpClient.generateFullPath(
            PathHeader::Auth,
            PathFooter::Login
        ),
        loginInformation
    );
}

String AuthService::logout() {
    return httpClient.post(
        httpClient.generateFullPath(
            PathHeader::Auth,
            PathFooter::Logout
        )
    );
}
