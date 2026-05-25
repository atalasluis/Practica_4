#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

class BarrierManager {
private:
    Servo servo;

    int servoPin;

    int openAngle;
    int closedAngle;

    bool barrierClosed;

public:
    BarrierManager(
        int pin,
        int openPos = 90,
        int closedPos = 0
    );

    void init();

    void open();

    void close();

    bool isClosed();
};