#include <Arduino.h>
#include "NetworkManager.h"
#include "AWSManager.h"
#include "UltrasonicSensor.h"
#include "SensorManager.h"
#include "SpeedManager.h"
#include "BarrierManager.h"
#include "AlarmManager.h"
#include "DisplayManager.h"
#include "StorageManager.h"

// ==========================================
// CONFIGURACIONES Y PARÁMETROS (HARDWARE/NUBE)
// ==========================================

const char* ssid     = "Galaxy A316AF1";
const char* password = "patatata";

const char* endpoint  = "a3pyx84p269dfq-ats.iot.us-east-1.amazonaws.com";
const char* thingName = "speed-02";

// Asignación de Pines GPIO
#define TRIG1 18
#define ECHO1 19
#define TRIG2 16
#define ECHO2 35

#define SDA_PIN 21
#define SCL_PIN 22

#define SERVO_PIN 13
#define BUZZER_PIN 32 
#define LED1_PIN 26
#define LED2_PIN 25

// ==========================================
// INSTANCIACIÓN DE MÓDULOS (ALTA COHESIÓN)
// ==========================================

NetworkManager wifi("RadarSpeed_Portal");
AWSManager     aws(endpoint, thingName);
SensorManager  sensors(TRIG1, ECHO1, TRIG2, ECHO2);
StorageManager storage;

SpeedManager   speedManager(sensors, 0.12, 10); // 12cm distancia, 10cm umbral
BarrierManager barrier(SERVO_PIN, 90, 0);
AlarmManager   alarmManager(BUZZER_PIN, LED1_PIN, LED2_PIN);
DisplayManager display(0x27, 16, 2);

// ==========================================
// ESTADO LOCAL DE CONTROL (MÁQUINA DE ESTADOS)
// ==========================================

unsigned long lastEventTime    = 0;
const int     eventCooldown    = 3000; // Bloqueo de rebotes (ms)

bool          localAlarmActive = false;
unsigned long localAlarmStart  = 0;
const unsigned long alarmDuration = 5000; // Duración de alerta local (ms)

// ==========================================
// DECLARACIÓN DE SUB-FUNCIONES (SRP)
// ==========================================

void handleNewMeasurement(float speed);
void handleAlarmTimeout();
void synchronizeActuators();
void printSystemMetrics(bool alarmState, bool barrierState);

// ==========================================
// SETUP SYSTEM
// ==========================================

void setup() {
    Serial.begin(115200);

    // 1. ARRANCAR LA PANTALLA DE INMEDIATO (Feedback visual para evitar sustos)
    display.init(SDA_PIN, SCL_PIN);
    display.setLine1("Iniciando...");
    display.setLine2("Buscando Red...");
    display.update();
    delay(500);

    // 2. LANZAR EL PORTAL / CONEXIÓN (Si se queda congelado aquí, al menos la pantalla ya avisó)
    wifi.connect();
    
    // 3. INICIALIZAR EL RESTO DEL HARDWARE (Sensores y Actuadores)
    aws.init();
    sensors.init();
    barrier.init();
    alarmManager.init();
    
    // 4. NOTIFICAR ÉXITO DEL ECOSYSTEMA
    display.setLine1("Sistema Online");
    display.setLine2("Listo para autos");
    display.update();
    
    delay(1000);
}

// ==========================================
// LOOP PRINCIPAL (ORQUESTADOR ORDENADO)
// ==========================================

void loop() {
    // Red y Nube
    if (wifi.isConnected()) {
        aws.loop();
    }
    wifi.reconnect();
    display.setWifiStatus(wifi.isConnected());

    // Actualización de Telemetría Física
    speedManager.update();

    // Verificación de paso de vehículo
    if (speedManager.hasNewMeasurement() && (millis() - lastEventTime > eventCooldown)) {
        lastEventTime = millis();
        handleNewMeasurement(speedManager.getSpeed());
    }

    // Máquina de estados temporal y sincronización
    handleAlarmTimeout();
    synchronizeActuators();

    // Renderizado y ejecución de Hardware
    alarmManager.update();
    display.update();
    
    delay(10);
}

// ==========================================
// IMPLEMENTACIÓN DE MÓDULOS DE CONTROL
// ==========================================

/**
 * Procesa el evento completo de paso de un vehículo.
 * Publica tanto al Shadow (Estado) como al Tópico Analítico (Evento) sin discriminar velocidad.
 */
void handleNewMeasurement(float speed) {
    int speedLimit = aws.desiredSpeedLimit;
    bool exceeded  = speed > speedLimit;

    // Actualización UI inmediata
    display.showSpeed(speed, speedLimit);

    if (exceeded) {
        localAlarmActive = true;
        localAlarmStart = millis();
        display.showAlarm();
    }

    // Resolución de prioridades Lógica de negocio (Local vs Nube)
    bool combinedAlarmState   = localAlarmActive || aws.remoteAlarm;
    bool combinedBarrierState = localAlarmActive || aws.remoteBarrier;

    printSystemMetrics(combinedAlarmState, combinedBarrierState);

    // Mapeo del estado del objeto
    DeviceState state;
    state.deviceId      = thingName;
    state.currentSpeed  = speed;
    state.speedLimit    = speedLimit;
    state.alarm         = combinedAlarmState;
    state.barrierClosed = combinedBarrierState;
    state.systemStatus  = exceeded ? "alarm" : "online";

    // Envío Dual a AWS (Pipeline Cloud asíncrono)
    if (wifi.isConnected() && aws.isConnected()) {
        aws.publishState(state); // 1. Actualiza el Shadow (Gemelo Digital)
        aws.publishEvent(state); // 2. Publicación de tráfico global para DynamoDB Analítico
    }
}

/**
 * Monitorea el temporizador de la alarma local para restaurar la normalidad del sistema.
 */
void handleAlarmTimeout() {
    if (localAlarmActive && (millis() - localAlarmStart >= alarmDuration)) {
        localAlarmActive = false;
        Serial.println("[SYSTEM] Alarma local finalizada. Restaurando estado.");

        DeviceState state;
        state.deviceId      = thingName;
        state.currentSpeed  = 0;
        state.speedLimit    = aws.desiredSpeedLimit;
        state.alarm         = aws.remoteAlarm;
        state.barrierClosed = aws.remoteBarrier;
        state.systemStatus  = "online";

        if (wifi.isConnected() && aws.isConnected()) {
            aws.publishState(state);
            aws.publishEvent(state); 
        }
    }
}

/**
 * Acopla los estados lógicos calculados con los actuadores físicos.
 */
void synchronizeActuators() {
    bool targetAlarm   = localAlarmActive || aws.remoteAlarm;
    bool targetBarrier = localAlarmActive || aws.remoteBarrier;

    if (targetAlarm) {
        alarmManager.activate();
    } else {
        alarmManager.deactivate();
    }

    if (targetBarrier) {
        barrier.close();
    } else {
        barrier.open();
    }
}

/**
 * Imprime métricas del sistema de forma limpia en el puerto serial.
 */
void printSystemMetrics(bool alarmState, bool barrierState) {
    Serial.println("\n--- [VEHICLE EVENT DETECTED] ---");
    Serial.printf("Actuator Alarm State:   %s\n", alarmState ? "ACTIVE" : "INACTIVE");
    Serial.printf("Actuator Barrier State: %s\n", barrierState ? "CLOSED" : "OPEN");
    Serial.println("--------------------------------");
}