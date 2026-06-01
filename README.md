# Practica_4

# 1. Requerimientos Funcionales y No Funcionales
* Funcionales
      
    * Detección de vehículos:El sistema deberá detectar la presencia de un vehículo mediante un sensor conectado al ESP32.

    * Medición de velocidad: El sistema deberá calcular la velocidad del vehículo en tiempo real a partir de los sensores instalados.

    * Comparación con límite de velocidad: El sistema deberá comparar la velocidad detectada con un límite de velocidad configurado remotamente.

    * Activación de alarma: El sistema deberá activar una alarma sonora cuando la velocidad del vehículo supere el límite establecido.

    * Control de barrera automática: El sistema deberá controlar un servomotor que actúa como barrera, cerrándola cuando se detecte exceso de velocidad.
  
* No Funcionales
      
    * Seguridad: Toda comunicación entre el ESP32 y AWS IoT Core deberá estar cifrada mediante TLS 1.2 usando certificados digitales.
     
    * Disponibilidad: El sistema deberá mantener conexión con AWS IoT Core con reconexión automática en caso de pérdida de conexión.
      
    * Eficiencia energética: El ESP32 deberá optimizar el uso de red evitando publicaciones innecesarias y reduciendo tráfico MQTT.
      
    * Mantenibilidad: El código deberá estar modularizado en componentes separados (sensor, AWSManager, actuadores) para facilitar mantenimiento.
  
# 2. Diseño del Sistema
* Diagrama de bloques
    
```mermaid 
    graph LR
        subgraph Adquisicion Entrada
            S1[Sensor Ultrasónico 1]
            S2[Sensor Ultrasónico 2]
        end
        
        subgraph Procesamiento Edge
            ESP[Microcontrolador ESP32\nSpeedManager]
        end
        
        subgraph Actuacion Salida
            L[Matriz LEDs / Alarma]
            D[Display LCD 16x2]
            B[Servo Motor Barrera]
        end
        
        S1 -->|Trigger/Echo| ESP
        S2 -->|Trigger/Echo| ESP
        ESP -->|PWM| B
        ESP -->|I2C| D
        ESP -->|GPIO| L
```

* Diagrama de circuito
    
![Diagrama del circuito](media/diagrama.png)

* Diagrama de arquitectura del sistema

```mermaid
    graph TD
    subgraph Frontend Conversacional
        A[Alexa Echo / App]
    end

    subgraph AWS Cloud Serverless
        L1[Lambda Orquestadora\nAlexa Backend]
        IoT[AWS IoT Core\nMessage Broker & Shadow]
        Rule[AWS IoT Rule\nspeed-monitor/data]
        L2[Lambda Ingestora\nSpeedDataIngestor]
        
        DB1[(DynamoDB\nuser_devices)]
        DB2[(DynamoDB\nspeed_events)]
    end

    subgraph Capa Edge
        ESP[Dispositivo ESP32]
    end

    A -- "Voz a Intent JSON" --> L1
    L1 -- "GetDevice(user_id)" --> DB1
    L1 -- "Query(Última Velocidad)" --> DB2
    L1 -- "Update Desired" --> IoT
    
    ESP -- "MQTT Publish Event" --> IoT
    IoT -- "Enruta payload" --> Rule
    Rule -- "Invoca" --> L2
    L2 -- "PutItem (Historial)" --> DB2
    
    ESP -->|Shadow Update| IoT
    IoT -->|Shadow Delta| ESP
``` 

* Diagramas estructurales y de comportamiento

```mermaid
     classDiagram
        class Main {
            +setup()
            +loop()
        }
        class SpeedManager {
            -SensorManager sensors
            -float sensorDistance
            -unsigned long t1
            -unsigned long t2
            +update()
            +hasNewMeasurement() bool
            +getSpeed() float
        }
        class SensorManager {
            +readDistances()
        }
        class AWSManager {
            -WiFiClientSecure net
            -PubSubClient client
            +publishEvent(DeviceState state)
            +publishState(DeviceState state)
        }
        class BarrierManager {
            +open()
            +close()
        }

        Main --> SpeedManager : Usa
        Main --> AWSManager : Usa
        Main --> BarrierManager : Controla
        SpeedManager --> SensorManager : Adquisición
```

```mermaid
     sequenceDiagram
        participant S as Sensores (Edge)
        participant E as ESP32 (Edge)
        participant IoT as AWS IoT Core
        participant Rule as IoT Rule
        participant L as Lambda Ingestora
        participant DB as DynamoDB (speed_events)

        S->>E: Vehículo detectado (T1 y T2)
        E->>E: Calcula Velocidad (V = d/t)
        alt V > SpeedLimit
            E->>E: Activa Alarma y cierra Barrera localmente
        end
        E->>IoT: Publish JSON a "speed-monitor/data"
        IoT->>Rule: Intercepta mensaje
        Rule->>L: Trigger de evento (Payload JSON)
        L->>L: Calcula hora, día y genera Timestamp
        L->>DB: PutItem (deviceId, timestamp, speed, etc)
        DB-->>L: 200 OK
```
* Diseño de la skill de Alexa
     
```mermaid
    stateDiagram-v2 
    [*] --> LaunchRequest : "Abre sistema de acceso"
    
    LaunchRequest --> LastSpeedIntent : "Cuál es la velocidad"
    LastSpeedIntent --> DynamoDB : Query
    DynamoDB --> RespuestaVoz
    
    LaunchRequest --> OpenBarrierIntent : "Abre la barrera"
    OpenBarrierIntent --> IoTShadow : Update Desired (open)
    IoTShadow --> RespuestaVoz
    
    LaunchRequest --> LedsIntent : "Enciende el LED rojo"
    LedsIntent --> IoTShadow : Update Desired (red)
    IoTShadow --> RespuestaVoz
    
    RespuestaVoz --> [*] : Cancel/Stop
    RespuestaVoz --> LaunchRequest : Reprompt
```

* Diseño de reportes (mockups) con información relevante para la toma de decisiones

```json
    {
        "state": {
            "desired": {
            "speedLimit": 10
            },
            "reported": {
            "deviceId": "speed-01",
            "systemStatus": "online",
            "currentSpeed": 3.98,
            "speedLimit": 10,
            "alarm": false,
            "barrier": "open"
            }
        }
    }
```

* Diseño del modelo de datos (tablas, tipos de datos, claves, etc.) para DynamoDB
    
```mermaid
erDiagram

    speed_events {
        string device_id
        int timestamp
        int speed
        bool exceeded
        int speedLimit
        bool barrierClosed
        bool alarmActivated
        int hour
        string day
    }

```
Descripción de las tablas

| Campo          | Tipo    | Clave | Descripción                                            |
| -------------- | ------- | ----- | ------------------------------------------------------ |
| device_id      | String  | PK    | Identificador del dispositivo que generó el evento.    |
| timestamp      | Number  | SK    | Marca temporal Unix Epoch del evento.                  |
| speed          | Number  | -     | Velocidad capturada por el sensor.                     |
| exceeded       | Boolean | -     | Indica si la velocidad excedió el límite configurado.  |
| speedLimit     | Number  | -     | Límite de velocidad vigente al momento de la medición. |
| barrierClosed  | Boolean | -     | Estado de la barrera durante el evento.                |
| alarmActivated | Boolean | -     | Estado de la alarma durante el evento.                 |
| hour           | Number  | -     | Hora del día utilizada para análisis y reportes.       |
| day            | String  | -     | Día de la semana asociado al evento.                   |


# 3. Implementación
  * Código fuente documentado (firmware del Objeto Inteligente y lógica del backend en las funciones Lambda)
 
  * Configuraciones en Alexa (Skill e Interaction Model)
   
  * Configuraciones en AWS (IoT Core, Rules, Lambda y DynamoDB)
   
# 4. Pruebas y Validaciones

# 5. Resultados

# 6. Conclusiones

# 7. Recomendaciones

# 8. Anexos
