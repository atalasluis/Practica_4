// WiFiManager.cpp

#include "WiFiManager.h"

WiFiManager::WiFiManager(
    const char* ssid,
    const char* password
) {

    this->ssid = ssid;
    this->password = password;

    lastReconnectAttempt = 0;
}

void WiFiManager::connect() {

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    Serial.println("Conectando WiFi...");
}

void WiFiManager::reconnect() {

    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (millis() - lastReconnectAttempt > 5000) {

        Serial.println("Reconectando WiFi...");

        WiFi.disconnect();

        WiFi.begin(ssid, password);

        lastReconnectAttempt = millis();
    }
}

bool WiFiManager::isConnected() {

    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIP() {

    if (!isConnected()) {
        return "0.0.0.0";
    }

    return WiFi.localIP().toString();
}