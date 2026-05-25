#include "StorageManager.h"

#include <ArduinoJson.h>

StorageManager::StorageManager() {
}

String StorageManager::buildEventJson(
    SpeedEvent event
) {

    StaticJsonDocument<512> doc;

    doc["deviceId"] = event.deviceId;

    doc["speed"] = event.speed;

    doc["speedLimit"] = event.speedLimit;

    doc["exceeded"] = event.exceeded;

    doc["barrierClosed"] =
        event.barrierClosed;

    doc["alarmActivated"] =
        event.alarmActivated;

    doc["systemStatus"] =
        event.systemStatus;

    doc["timestamp"] =
        event.timestamp;

    String output;

    serializeJson(doc, output);

    return output;
}