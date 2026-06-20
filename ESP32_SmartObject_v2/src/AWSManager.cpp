#include "AWSManager.h"

#include <ArduinoJson.h>

#include "certs/root_ca.h"
#include "certs/device_cert.h"
#include "certs/private_key.h"

AWSManager* AWSManager::instance = nullptr;

AWSManager::AWSManager(
    const char* endpoint,
    const char* thingName
) 
: client(net),
  endpoint(endpoint),
  thingName(thingName) {

    instance = this;

    desiredSpeedLimit = 3;///limite de velocidad (km/h)
    remoteAlarm = false;
    remoteBarrier = false;

    updateTopic =
        "$aws/things/" +
        String(thingName) +
        "/shadow/update";

    deltaTopic =
        "$aws/things/" +
        String(thingName) +
        "/shadow/update/delta";
}

void AWSManager::init() {

    net.setCACert(AWS_ROOT_CA);
    net.setCertificate(DEVICE_CERT);
    net.setPrivateKey(PRIVATE_KEY);

    client.setServer(endpoint, 8883);

    client.setCallback(callbackStatic);

    client.setKeepAlive(60);
}

void AWSManager::connect() {

    if(client.connected()) {
        return;
    }

    Serial.println("Conectando AWS IoT...");

    if(client.connect(thingName)) {

        Serial.println("AWS conectado");

        String topic =
            "$aws/things/" +
            String(thingName) +
            "/shadow/update/delta";

        client.subscribe(topic.c_str());

        Serial.println("Shadow subscribed");

    } else {

        Serial.print("AWS ERROR rc=");

        Serial.println(client.state());
    }
}

void AWSManager::loop() {
    static unsigned long lastReconnect = 0;

    if (
        !client.connected()
        &&
        millis() - lastReconnect > 5000
    ) {

        lastReconnect = millis();

        connect();
    }

    client.loop();
}

void AWSManager::callbackStatic(
    char* topic,
    byte* payload,
    unsigned int length
) {

    if (instance) {
        instance->callback(topic, payload, length);
    }
}

void AWSManager::callback(
    char* topic,
    byte* payload,
    unsigned int length
) {

    Serial.println("MENSAJE MQTT RECIBIDO");

    Serial.print("TOPIC: ");
    Serial.println(topic);

    String message;

    for(int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.println(message);

    StaticJsonDocument<256> doc;

    DeserializationError error =
        deserializeJson(doc, payload, length);

    if (error) return;

    if (doc["state"].containsKey("speedLimit")) {

        desiredSpeedLimit =
            doc["state"]["speedLimit"];

        Serial.print("Nuevo limite: ");
        Serial.println(desiredSpeedLimit);
    }

    if(doc["state"].containsKey("alarm")) {

        remoteAlarm =
            doc["state"]["alarm"];
        Serial.print("Remote alarm: ");
        Serial.println(remoteAlarm);

        DeviceState state;

        state.deviceId = "speed-01";
        state.currentSpeed = 0;
        state.speedLimit = desiredSpeedLimit;
        state.alarm = remoteAlarm;
        state.barrierClosed = remoteBarrier;
        state.systemStatus = "online";

        publishState(state);
    }

    if(doc["state"].containsKey("barrier")) {

        remoteBarrier =
            doc["state"]["barrier"];
        Serial.print("Remote barrier: ");
        Serial.println(remoteBarrier);

        DeviceState state;

        state.deviceId = "speed-01";
        state.currentSpeed = 0;
        state.speedLimit = desiredSpeedLimit;
        state.alarm = remoteAlarm;
        state.barrierClosed = remoteBarrier;
        state.systemStatus = "online";

        publishState(state);

    }
    Serial.println("------ ESTADO AWS ------");

    Serial.print("desiredSpeedLimit = ");
    Serial.println(desiredSpeedLimit);

    Serial.print("remoteAlarm = ");
    Serial.println(remoteAlarm);

    Serial.print("remoteBarrier = ");
    Serial.println(remoteBarrier);

    Serial.println("------------------------");
}

void AWSManager::publishState(DeviceState state) {

    StaticJsonDocument<256> doc;

    doc["state"]["reported"]["deviceId"] =
        state.deviceId;

    doc["state"]["reported"]["currentSpeed"] =
        state.currentSpeed;

    doc["state"]["reported"]["speedLimit"] =
        state.speedLimit;

    doc["state"]["reported"]["alarm"] =
        state.alarm;

    doc["state"]["reported"]["barrier"] =
        state.barrierClosed;

    doc["state"]["reported"]["systemStatus"] =
        state.systemStatus;

    char buffer[512];

    serializeJson(doc, buffer);

    bool ok = client.publish(
        updateTopic.c_str(),
        buffer
    );

    if(ok) {

        Serial.println("Shadow actualizado");

    } else {

        Serial.println("ERROR publicando shadow");
    }
}

void AWSManager::publishEvent(DeviceState state) {

    StaticJsonDocument<256> doc;

    doc["currentSpeed"] = state.currentSpeed;
    doc["speedLimit"] = state.speedLimit;
    doc["alarm"] = state.alarm;

    char buffer[256];
    serializeJson(doc, buffer);

    bool ok = client.publish(
        "speed-monitor/data",
        buffer
    );

    if(ok) {
        Serial.println("Evento enviado a DynamoDB!");
    } else {
        Serial.println("ERROR publicando evento analítico");
    }
}

bool AWSManager::isConnected() {

    return client.connected();
}