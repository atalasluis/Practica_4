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

    Serial.print("Conectando WiFi");

    int retries = 0;

    while(
        WiFi.status() != WL_CONNECTED &&
        retries < 20
    ) {

        delay(500);

        Serial.print(".");

        retries++;
    }

    Serial.println();

    if(WiFi.status() == WL_CONNECTED) {

        Serial.println("WiFi conectado");

        Serial.print("IP: ");

        Serial.println(WiFi.localIP());

    } else {

        Serial.println("ERROR WIFI");
    }
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