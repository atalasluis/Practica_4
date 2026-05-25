#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class DisplayManager {
private:
    LiquidCrystal_I2C lcd;

    String line1;
    String line2;

    bool wifiConnected;

    unsigned long lastUpdate;

public:
    DisplayManager(
        uint8_t address,
        uint8_t cols,
        uint8_t rows
    );

    void init(uint8_t sda, uint8_t scl);

    void setLine1(String text);

    void setLine2(String text);

    void setWifiStatus(bool status);

    void showSpeed(
        float speed,
        int limit
    );

    void showAlarm();

    void showNormal();

    void update();
};