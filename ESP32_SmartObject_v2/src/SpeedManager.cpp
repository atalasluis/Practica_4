#include "SpeedManager.h"

SpeedManager::SpeedManager(
    SensorManager& sensorManager,
    float distanceMeters,
    int threshold
)
: sensors(sensorManager) {

    sensorDistance = distanceMeters;

    thresholdDistance = threshold;

    waitingSecondSensor = false;

    t1 = 0;
    t2 = 0;

    lastSpeed = -1;
}

void SpeedManager::update() {

    long d1 = sensors.getDistance1Fast();
    long d2 = sensors.getDistance2Fast();

    // DEBUG
    Serial.print("D1: ");
    Serial.print(d1);

    Serial.print(" | D2: ");
    Serial.println(d2);

    // =====================
    // SENSOR 1
    // =====================

    if (
        !waitingSecondSensor &&
        d1 > 0 &&
        d1 < thresholdDistance
    ) {

        t1 = micros();

        waitingSecondSensor = true;

        return;
    }

    // =====================
    // TIMEOUT
    // =====================

    if (
        waitingSecondSensor &&
        micros() - t1 > 2000000
    ) {

        waitingSecondSensor = false;

        return;
    }

    // =====================
    // SENSOR 2
    // =====================

    if (
        waitingSecondSensor &&
        d2 > 0 &&
        d2 < thresholdDistance
    ) {

        t2 = micros();

        float deltaTime =
            (t2 - t1) / 1000000.0;

        waitingSecondSensor = false;

        // INVALIDO
        if(deltaTime <= 0) {
            return;
        }

        // MUY RAPIDO = ERROR
        if(deltaTime < 0.05) {
            return;
        }

        float speedMS =
            sensorDistance / deltaTime;

        float speedKMH =
            speedMS * 3.6;

        // FILTRO MAXIMO
        if(speedKMH > 120) {
            return;
        }

        // FILTRO MINIMO
        if(speedKMH < 2) {
            return;
        }

        lastSpeed = speedKMH;

        Serial.print("SPEED: ");
        Serial.println(lastSpeed);
    }
}

bool SpeedManager::hasNewMeasurement() {

    return lastSpeed > 0;
}

float SpeedManager::getSpeed() {

    float temp = lastSpeed;

    lastSpeed = -1;

    return temp;
}

void SpeedManager::reset() {

    waitingSecondSensor = false;

    lastSpeed = -1;
}