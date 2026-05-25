// UltrasonicSensor.cpp

#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(
    int trig,
    int echo,
    long timeoutUS
) {

    trigPin = trig;
    echoPin = echo;

    timeout = timeoutUS;
}

void UltrasonicSensor::init() {

    pinMode(trigPin, OUTPUT);

    pinMode(echoPin, INPUT);

    digitalWrite(trigPin, LOW);
}

float UltrasonicSensor::medirSimple() {

    // Pulso trigger
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin, LOW);

    // Leer eco
    long duration =
        pulseIn(
            echoPin,
            HIGH,
            timeout
        );

    // Timeout
    if (duration == 0) {
        return -1;
    }

    // Distancia cm
    float distance =
        (duration * 0.0343) / 2.0;

    return distance;
}

void UltrasonicSensor::ordenar(
    float arr[],
    int n
) {

    for (int i = 0; i < n - 1; i++) {

        for (int j = 0; j < n - i - 1; j++) {

            if (arr[j] > arr[j + 1]) {

                float temp = arr[j];

                arr[j] = arr[j + 1];

                arr[j + 1] = temp;
            }
        }
    }
}

long UltrasonicSensor::medir() {

    const int samples = 7;

    float values[samples];

    int validSamples = 0;

    for (int i = 0; i < samples; i++) {

        float d = medirSimple();

        if (d > 0 && d < 400) {

            values[validSamples++] = d;
        }

        delay(10);
    }

    if (validSamples == 0) {
        return -1;
    }

    ordenar(values, validSamples);

    float median =
        values[validSamples / 2];

    return (long)median;
}

float UltrasonicSensor::medirFast() {

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin, LOW);

    long duration =
        pulseIn(
            echoPin,
            HIGH,
            10000
        );

    if(duration == 0) {
        return -1;
    }

    float distance =
        (duration * 0.0343) / 2.0;

    return distance;
}