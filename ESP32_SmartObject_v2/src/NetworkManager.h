#pragma once

#include <WiFi.h>

class NetworkManager {
private:
    const char* portalSSID;
    unsigned long lastReconnectAttempt;

    unsigned long lossTime; 
    bool          trackingLoss;

public:
    NetworkManager(
        const char* apName="Vias_Speed_Portal"
    );

    void connect();

    void reconnect();

    bool isConnected();

    String getIP();
};