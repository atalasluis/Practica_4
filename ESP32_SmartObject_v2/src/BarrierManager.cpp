#include "BarrierManager.h"

BarrierManager::BarrierManager(
    int pin,
    int openPos,
    int closedPos
) {

    servoPin = pin;

    openAngle = openPos;
    closedAngle = closedPos;

    barrierClosed = false;
}

void BarrierManager::init() {

    servo.setPeriodHertz(50);
    servo.attach(servoPin, 500, 2400);

    open();
}

void BarrierManager::open() {

    servo.write(openAngle);

    barrierClosed = false;
}

void BarrierManager::close() {

    servo.write(closedAngle);

    barrierClosed = true;
}

bool BarrierManager::isClosed() {
    return barrierClosed;
}