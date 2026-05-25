#pragma once

#include "SensorManager.h"

class SpeedManager {
private:
    SensorManager& sensors;

    float sensorDistance; // metros

    bool waitingSecondSensor;

    unsigned long t1;
    unsigned long t2;

    float lastSpeed;

    int thresholdDistance;

public:
    SpeedManager(
        SensorManager& sensorManager,
        float distanceMeters,
        int threshold = 15
    );

    void update();

    bool hasNewMeasurement();

    float getSpeed();

    void reset();
};