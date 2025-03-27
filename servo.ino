#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Servo.h>

#include <Pulses.h>

// Constantes de configuración
#define MAX_TOPIC_LENGTH 25
const char ID_DEVICE[] = "D_0001";

const int precision = 3;
const unsigned long pulseDuration = 50000;// Duración del movimiento del motor
// MAC address para el controlador Ethernet
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Detalles del broker MQTT - ACTUALIZADOS PARA NODE-RED
const char broker[] = "85.208.22.48"; // Dirección IP del broker Node-RED
const int port = 1883; // Puerto del broker Node-RED
const char mqttUsername[] = "admin";      
const char mqttPassword[] = "admin";      

Servo servo;

// Inicializar variables de temas MQTT
#include <avr/pgmspace.h>
const char* topicInit = "/init";
const char* topicComIn = "/s6htEcBy4EaAfsZthW2e/toPLC";
const char* topicComOut = "/s6htEcBy4EaAfsZthW2e/return";

bool REGISTERED = false;

// Inicializar Ethernet y cliente MQTT
EthernetClient ethClient;
PubSubClient mqtt(ethClient);

#define LED_PIN 13
#define LED_OPENING Q0_4
#define LED_CLOSING Q0_2

int ledStatus = LOW;
uint32_t lastReconnectAttempt = 0;
uint32_t lastPublishTime = 0; // Tiempo de la última publicación
const uint32_t publishInterval = 180000; // Intervalo de publicación (3 minutos)

unsigned long pulseStartTime = 0; // Almacenar el tiempo en que comenzó el pulso
bool isPulsing = false; // Estado de la señal PWM

void stopAllTasks() {
  // Detener cualquier tarea en ejecución
  digitalWrite(Q0_4, LOW);
  digitalWrite(Q0_2, LOW);
  isPulsing = false;
  mqtt.publish(topicComOut, "stopped"); // Informar que la tarea se ha detenido
  Serial.println("Todas las tareas detenidas");
}


void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  payload[len] = '\0'; // Asegurar que el payload esté terminado en null
  Serial.println((char*)payload);

  // Proceder solo si el tema del mensaje entrante coincide
  if (String(topic) == String(topicComIn)) {
    Serial.println("Listo para abrir o cerrar.");

    // Activación de motores y gestión de estados
    // Orden de abrir
    if (payload[0] == '1') {
      digitalWrite(Q0_4, HIGH);
      Serial.println("abriendose");
      pulseStartTime = millis();
      isPulsing = true;
      mqtt.publish(topicComOut, "opening...");//abriendose
    } 
    // Orden de cerrar
    else if (payload[0] == '0') {
      digitalWrite(Q0_2, HIGH);
      Serial.println("cerrandose");
      pulseStartTime = millis(); // Registrar el tiempo de inicio
      isPulsing = true; // Indicar que el pulso está activo
      mqtt.publish(topicComOut, "closing...");
    } 
    //Orden de parar
    else if(payload[0] =='2'){
      stopAllTasks();
    }
  }
  else{
    Serial.println("El mensaje no coincide con ningun topic registrado.");
  }
}

boolean mqttConnect() {
  Serial.print("Conectando a ");
  Serial.print(broker);
  Serial.print(":");
  Serial.println(port);
  
  // Conectarse al broker MQTT con autenticación
  boolean status = mqtt.connect("PLC", mqttUsername, mqttPassword);

  if (status == false) {
    Serial.print("Falló: ");
    Serial.println(mqtt.state());
    return false;
  }

  Serial.println("Conectado");

  if (REGISTERED == false) {
    mqtt.publish(topicInit, ID_DEVICE);
    REGISTERED = true;
  }

  mqtt.subscribe(topicComIn);
  return mqtt.connected();
}

void setup() {
  // Configurar la velocidad de la consola
  Serial.begin(115200);
  pinMode(Q0_4, OUTPUT);
  pinMode(Q0_2, OUTPUT);
  pinMode(Q0_3, OUTPUT);
  
  Serial.println("Iniciando");
  delay(10);

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_OPENING, OUTPUT);
  pinMode(LED_CLOSING, OUTPUT);

  Serial.println("Esperando");

  Ethernet.init(10); // La mayoría de los shields de Arduino
  Ethernet.begin(mac); // Reemplaza 'mac' con tu dirección MAC de Ethernet
  delay(1000); // Permitir tiempo para inicializar Ethernet

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("No se encontró el shield Ethernet. No puede funcionar sin hardware. :(");
    while (true) {
      delay(1); // No hacer nada, no tiene sentido correr sin hardware Ethernet
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("El cable Ethernet no está conectado.");
  }

  // Configurar el broker MQTT con la nueva dirección y puerto
  mqtt.setServer(broker, port);
  mqtt.setCallback(mqttCallback);

  // Imprimir las rutas de los topics para comprobar
  Serial.println("Rutas de los topics:");
  Serial.print("topicInit: ");
  Serial.println(topicInit);
  Serial.print("topicComIn: ");
  Serial.println(topicComIn);
  Serial.print("topicComOut: ");
  Serial.println(topicComOut);
}

void loop() {
  if (!mqtt.connected()) {
    Serial.println("=== MQTT NO CONECTADO ===");
    delay(1000);

    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }

  mqtt.loop();
  // Verificar si han pasado 8 segundos desde que se inició el pulso
  if (isPulsing && (millis() - pulseStartTime >= pulseDuration)) {
    isPulsing = false; // Indicar que el pulso ha terminado

    if (digitalRead(LED_OPENING) == HIGH) {
      digitalWrite(LED_OPENING, LOW);
      mqtt.publish(topicComOut, "opened"); // Abierto
      Serial.println("Motor detenido después de 8 segundos (abierto)");
    }

    if (digitalRead(LED_CLOSING) == HIGH) {
      digitalWrite(LED_CLOSING, LOW);
      mqtt.publish(topicComOut, "closed"); // Cerrado
      Serial.println("Motor detenido después de 8 segundos (cerrado)");
    }
  }
}