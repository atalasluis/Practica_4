#pragma once

#include "UltrasonicSensor.h"

class SensorManager {
private:
    UltrasonicSensor sensor1;
    UltrasonicSensor sensor2;

    unsigned long lastReadTime;
    const int readDelay = 30; // ms entre sensores

public:
    SensorManager(
        int trig1, int echo1,
        int trig2, int echo2
    );

    void init();

    long getDistance1();
    long getDistance2();
};