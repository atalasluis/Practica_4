#include "SensorManager.h"

SensorManager::SensorManager(
    int trig1, int echo1,
    int trig2, int echo2
)
: sensor1(trig1, echo1),
  sensor2(trig2, echo2) {

    lastReadTime = 0;
}

void SensorManager::init() {
    sensor1.init();
    sensor2.init();
}

long SensorManager::getDistance1() {
    return sensor1.medir();
}

long SensorManager::getDistance2() {

    // Evita interferencia ultrasónica
    if (millis() - lastReadTime < readDelay) {
        delay(readDelay);
    }

    lastReadTime = millis();

    return sensor2.medir();
}