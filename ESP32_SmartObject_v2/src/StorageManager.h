#pragma once

#include <Arduino.h>

struct SpeedEvent {
    String deviceId;

    //float speed;
    int speedLimit;

    bool exceeded;

    bool barrierClosed;
    bool alarmActivated;

    String systemStatus;

    unsigned long timestamp;
};

class StorageManager {
public:
    StorageManager();

    String buildEventJson(SpeedEvent event);
};