#pragma once

#include <WiFi.h>

class WiFiManager {
private:
    const char* ssid;
    const char* password;

    unsigned long lastReconnectAttempt;

public:
    WiFiManager(
        const char* ssid,
        const char* password
    );

    void connect();

    void reconnect();

    bool isConnected();

    String getIP();
};