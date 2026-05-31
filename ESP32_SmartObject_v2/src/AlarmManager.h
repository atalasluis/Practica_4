#pragma once

#include <Arduino.h>

class AlarmManager {
private:
    int buzzerPin;

    int led1Pin;
    int led2Pin;

    bool active;

    unsigned long lastBlink;
    bool blinkState;

    int blinkInterval;

public:
    AlarmManager(
        int buzzer,
        int led1,
        int led2
    );

    void init();

    void activate();

    void deactivate();

    void update();

    bool isActive();
    
};