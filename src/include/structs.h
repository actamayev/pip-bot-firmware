#pragma once
#include <Arduino.h>

struct WiFiCredentials {
	String ssid;
	String password;
};

struct MessageTokens {
    int eventIndex = -1;
    int chunkIndexIndex = -1;
    int totalChunksIndex = -1;
    int totalSizeIndex = -1;
    int isLastIndex = -1;
    int dataIndex = -1;
};
