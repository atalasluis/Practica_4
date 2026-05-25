#include "DisplayManager.h"

#include <Wire.h>

DisplayManager::DisplayManager(
    uint8_t address,
    uint8_t cols,
    uint8_t rows
)
: lcd(address, cols, rows) {

    line1 = "";
    line2 = "";

    wifiConnected = false;

    lastUpdate = 0;
}

void DisplayManager::init(
    uint8_t sda,
    uint8_t scl
) {

    Wire.begin(sda, scl);

    lcd.init();

    lcd.backlight();

    lcd.clear();
}

void DisplayManager::setLine1(String text) {

    line1 = text;
}

void DisplayManager::setLine2(String text) {

    line2 = text;
}

void DisplayManager::setWifiStatus(bool status) {

    wifiConnected = status;
}

void DisplayManager::showSpeed(
    float speed,
    int limit
) {

    line1 =
        "Speed:" +
        String(speed, 1) +
        " km/h";

    line2 =
        "Limit:" +
        String(limit) +
        " ";

    if (wifiConnected) {
        line2 += "WiFi";
    } else {
        line2 += "OFF";
    }
}

void DisplayManager::showAlarm() {

    line2 = "ALARM ACTIVE";
}

void DisplayManager::showNormal() {

    line2 = "SYSTEM NORMAL";
}

void DisplayManager::update() {

    if (millis() - lastUpdate < 200) {
        return;
    }

    lastUpdate = millis();

    lcd.setCursor(0, 0);

    lcd.print("                ");

    lcd.setCursor(0, 0);

    lcd.print(line1);

    lcd.setCursor(0, 1);

    lcd.print("                ");

    lcd.setCursor(0, 1);

    lcd.print(line2);
}