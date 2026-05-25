#include "AlarmManager.h"

AlarmManager::AlarmManager(
    int buzzer,
    int led1,
    int led2
) {

    buzzerPin = buzzer;

    led1Pin = led1;
    led2Pin = led2;

    active = false;

    lastBlink = 0;
    blinkState = false;

    blinkInterval = 300;
}

void AlarmManager::init() {

    pinMode(buzzerPin, OUTPUT);

    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);

    deactivate();
}

void AlarmManager::activate() {

    active = true;

    digitalWrite(buzzerPin, LOW);
}

void AlarmManager::deactivate() {

    active = false;

    digitalWrite(buzzerPin, HIGH);

    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
}

void AlarmManager::update() {

    if (!active) return;

    if (millis() - lastBlink > blinkInterval) {

        lastBlink = millis();

        blinkState = !blinkState;

        digitalWrite(led1Pin, blinkState);
        digitalWrite(led2Pin, blinkState);
    }
}

bool AlarmManager::isActive() {
    return active;
}