#pragma once

#include <Arduino.h>

class UltrasonicSensor {
private:
    int trigPin;
    int echoPin;

    long timeout;

    float medirSimple();

    void ordenar(
        float arr[],
        int n
    );

public:
    UltrasonicSensor(
        int trig,
        int echo,
        long timeoutUS = 25000
    );

    void init();

    long medir();
};