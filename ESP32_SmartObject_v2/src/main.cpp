// main.cpp

#include <Arduino.h>

#include "WiFiManager.h"
#include "AWSManager.h"

#include "UltrasonicSensor.h"
#include "SensorManager.h"
#include "SpeedManager.h"

#include "BarrierManager.h"
#include "AlarmManager.h"
#include "DisplayManager.h"

#include "StorageManager.h"

// =====================
// WIFI
// =====================

//const char* ssid = "Galaxy A316AF1";
//const char* password = "patatata";

//const char* ssid = "TP-Link_22C2";
//const char* password = "69562495";

//const char* ssid = "TP-Link_Invitados";
//const char* password = "kebHp50A";

const char* ssid = "Susana";
const char* password = "12345678";
// =====================
// AWS
// =====================

const char* endpoint =
"a3pyx84p269dfq-ats.iot.us-east-1.amazonaws.com";

const char* thingName =
"speed-01";

// =====================
// HC-SR04
// =====================

// SENSOR 1
#define TRIG1 18
#define ECHO1 19

// SENSOR 2
#define TRIG2 16
#define ECHO2 35

// =====================
// DISPLAY I2C
// =====================

#define SDA_PIN 21
#define SCL_PIN 22

// =====================
// SERVO
// =====================

#define SERVO_PIN 13

// =====================
// BUZZER + LEDS
// =====================

#define BUZZER_PIN 32 

#define LED1_PIN 26
#define LED2_PIN 25

// =====================
// OBJETOS
// =====================

WiFiManager wifi(
    ssid,
    password
);

AWSManager aws(
    endpoint,
    thingName
);

SensorManager sensors(
    TRIG1, ECHO1,
    TRIG2, ECHO2
);

SpeedManager speedManager(
    sensors,
    0.12, // distancia entre sensores (m)
    10    // umbral detección (cm)
);

BarrierManager barrier(
    SERVO_PIN,
    90,
    0
);

AlarmManager alarmManager(
    BUZZER_PIN,
    LED1_PIN,
    LED2_PIN
);

DisplayManager display(
    0x27,
    16,
    2
);

StorageManager storage;

// =====================
// CONTROL
// =====================

unsigned long lastEventTime = 0;

const int eventCooldown = 3000;

bool localAlarmActive = false;

unsigned long localAlarmStart = 0;

const unsigned long alarmDuration = 5000;

// =====================
// SETUP
// =====================

void setup() {

    Serial.begin(115200);

    // WIFI
    wifi.connect();
    delay(3000);
    Serial.println("Continuando setup...");
    // AWS
    aws.init();
    //aws.connect();
    // SENSORES
    sensors.init();

    // DISPLAY
    display.init(
        SDA_PIN,
        SCL_PIN
    );

    // SERVO
    barrier.init();

    // ALARMA
    alarmManager.init();

    display.setLine1("System Init");
    display.setLine2("Starting...");
    display.update();

    delay(1000);
}

// =====================
// LOOP
// =====================

void loop() {

    // =====================
    // AWS
    // =====================

    if (wifi.isConnected()) {

        aws.loop(); // SIEMPRE
    }

    // =====================
    // WIFI
    // =====================

    wifi.reconnect();

    display.setWifiStatus(
        wifi.isConnected()
    );



    // =====================
    // SPEED
    // =====================

    speedManager.update();

    // =====================
    // NUEVA MEDICION
    // =====================

    if (
        speedManager.hasNewMeasurement()
        &&
        millis() - lastEventTime >
        eventCooldown
    ) {

        lastEventTime = millis();

        float speed =
            speedManager.getSpeed();

        int speedLimit =
            aws.desiredSpeedLimit;

        bool exceeded =
            speed > speedLimit;

        // DISPLAY

        display.showSpeed(
            speed,
            speedLimit
        );

        // ALARMA

        if(exceeded) {

            localAlarmActive = true;

            localAlarmStart = millis();

            display.showAlarm();
        }
        bool alarmState =
            localAlarmActive ||
            aws.remoteAlarm;

        bool barrierState =
            localAlarmActive ||
            aws.remoteBarrier;
            
        Serial.println("=== LOOP ===");
        Serial.print("alarmState=");
        Serial.println(alarmState);

        Serial.print("barrierState=");
        Serial.println(barrierState);
        Serial.println("============");

        // STATE

        DeviceState state;

        state.deviceId = "speed-01";

        state.currentSpeed = speed;

        state.speedLimit = speedLimit;

        state.alarm = alarmState;

        state.barrierClosed = barrierState;

        state.systemStatus =
            exceeded ? "alarm"
                      : "online";

        // PUBLICAR

        if (
            wifi.isConnected()
            &&
            aws.isConnected()
        ) {

            aws.publishState(state);
        }

        // EVENTO

        SpeedEvent event;

        event.deviceId =
            thingName;

        event.speedLimit =
            speedLimit;

        event.exceeded =
            exceeded;

        event.barrierClosed =
            barrierState;

        event.alarmActivated =
            alarmState;

        event.systemStatus =
            state.systemStatus;

        event.timestamp =
            millis();

        String json =
            storage.buildEventJson(event);

        Serial.println("EVENT:");
        Serial.println(json);
    }


    // =====================
    // TEMPORIZADOR ALARMA
    // =====================

    if(
        localAlarmActive &&
        millis() - localAlarmStart >= alarmDuration
    ) {

        localAlarmActive = false;

        Serial.println("Alarma local finalizada");

        DeviceState state;

        state.deviceId = "speed-01";
        state.currentSpeed = 0;
        state.speedLimit = aws.desiredSpeedLimit;
        state.alarm = aws.remoteAlarm;
        state.barrierClosed = aws.remoteBarrier;
        state.systemStatus = "online";

        if(
            wifi.isConnected() &&
            aws.isConnected()
        ) {
            aws.publishState(state);
        }
    }

    // CALCULAR ESTADOS AQUÍ

    bool alarmState =
        localAlarmActive ||
        aws.remoteAlarm;

    bool barrierState =
        localAlarmActive ||
        aws.remoteBarrier;

    Serial.println("=== LOOP ===");
    Serial.print("alarmState=");
    Serial.println(alarmState);

    Serial.print("barrierState=");
    Serial.println(barrierState);
    Serial.println("============");

    if(alarmState) {

        alarmManager.activate();

    } else {

        alarmManager.deactivate();
    }

    if(barrierState) {

        barrier.close();

    } else {

        barrier.open();
    }

    // =====================
    // UPDATE
    // =====================

    alarmManager.update();

    display.update();


    delay(10);
}