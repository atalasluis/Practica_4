// NetworkManager.cpp

#include "NetworkManager.h"
#include <WiFiManager.h>

NetworkManager::NetworkManager(const char* apName) {
    this->portalSSID = apName;
    this->lastReconnectAttempt = 0;
    this->lossTime = 0;
    this->trackingLoss = false;
}

void NetworkManager::connect() {
    WiFi.mode(WIFI_STA);
    ::WiFiManager wm;
    
    wm.setDebugOutput(true);
    wm.setConnectTimeout(15);       // Bajamos a 15s en el arranque para que sea más ágil
    wm.setConfigPortalTimeout(120); 

    Serial.println("\n[NETWORK] Buscando credenciales de red en memoria NVS...");

    // autoConnect solo se usa al encender la placa por primera vez
    if (!wm.autoConnect(portalSSID)) {
        Serial.println("[NETWORK] Tiempo límite del portal agotado. Reiniciando hardware...");
        delay(3000);
        ESP.restart(); 
    }

    Serial.println("[NETWORK] Conexión Wi-Fi establecida exitosamente.");
    Serial.print("[NETWORK] Dirección IP asignada: ");
    Serial.println(WiFi.localIP());

    trackingLoss = false;   
}

void NetworkManager::reconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        trackingLoss = false; 
        return;
    }

    // Primer instante de la caída
    if (!trackingLoss) {
        lossTime = millis(); 
        trackingLoss = true;
        Serial.println("\n[NETWORK] ¡Alerta! Conexión Wi-Fi perdida.");
        Serial.println("[NETWORK] Iniciando ventana de tolerancia de 30 segundos...");
    }

    // =================================================================
    // ¡CORRECCIÓN MAESTRA!: TOLERANCIA AGOTADA
    // =================================================================
    if (millis() - lossTime > 30000) {
        Serial.println("\n[NETWORK] Tolerancia agotada (30s sin red).");
        Serial.println("[NETWORK] Forzando Portal Cautivo Inmediato...");
        
        // 1. Matamos cualquier intento previo de conexión para liberar el chip
        WiFi.disconnect(true); 
        delay(100);
        
        ::WiFiManager wm;
        wm.setDebugOutput(true);
        wm.setConfigPortalTimeout(90); // 1.5 minutos para reconfigurar o se apaga
        
        // 2. startConfigPortal levanta el AP AL INSTANTE sin intentar conectar a nada antes
        if (wm.startConfigPortal(portalSSID)) {
            Serial.println("[NETWORK] Red reconfigurada con éxito desde el portal.");
            trackingLoss = false;
        } else {
            Serial.println("[NETWORK] El portal expiró. Intentando ciclo normal...");
            lossTime = millis(); // Reseteamos el reloj para dar otros 30s de gracia
        }
        return;
    }

    // Intentos silenciosos intermedios (cada 10 segundos) mientras no se venzan los 30s
    if (millis() - lastReconnectAttempt > 10000) {
        lastReconnectAttempt = millis();
        Serial.println("[NETWORK] Enlace Wi-Fi perdido. Intentando reconexión silenciosa...");
        WiFi.begin(); // Quitamos el disconnect de aquí para no jamear el driver cada 10s
    }
}

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String NetworkManager::getIP() {
    if (!isConnected()) {
        return "0.0.0.0";
    }
    return WiFi.localIP().toString();
}