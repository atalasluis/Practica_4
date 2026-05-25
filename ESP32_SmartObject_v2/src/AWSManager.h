#pragma once

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

struct DeviceState {
    String deviceId;
    float currentSpeed;
    int speedLimit;

    bool alarm;
    bool barrierClosed;

    String systemStatus;
};

class AWSManager {
private:
    WiFiClientSecure net;
    PubSubClient client;

    

    const char* endpoint;
    const char* thingName;

    String updateTopic;
    String deltaTopic;

    static AWSManager* instance;

    static void callbackStatic(
        char* topic,
        byte* payload,
        unsigned int length
    );

    void callback(
        char* topic,
        byte* payload,
        unsigned int length
    );

public:
    AWSManager(
        const char* endpoint,
        const char* thingName
    );

    void init();

    void connect();

    void loop();

    void publishState(DeviceState state);

    int desiredSpeedLimit;

    bool isConnected(); 
};
