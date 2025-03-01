#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Servo.h>

#include <Pulses.h>

// Constantes de configuración
#define MAX_TOPIC_LENGTH 25
const bool SHORT_LON = true;
const char ID_DEVICE[] = "D_0001";

const unsigned long frequencyStartOpen = 1000;//frequencia para que empiece a girara
const unsigned long frequencyStartClockwise = 1800;
const unsigned long frequencyStop = 1400;//0UL;
const int precision = 3;
const unsigned long pulseDuration = 8000;// Duración del movimiento del motor
// MAC address para el controlador Ethernet
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Detalles del broker MQTT
const char broker[] = "node02.myqtthub.com"; // MyQttHub broker
const char mqttUsername[] = "juikmon11@gmail.com";      
const char mqttPassword[] = "CdCxL9hEAj37H5a";      

Servo servo;

// Inicializar variables de temas MQTT
#include <avr/pgmspace.h>
const char* topicInit = "Gsm/init";
const char* topicHum = "Gsm/2h8Cg9KvNhgWTdbmZsPo/HumFromPlc";
const char* topicTemp = "Gsm/2h8Cg9KvNhgWTdbmZsPo/TempFromPlc";
const char* topicComIn = "Gsm/2h8Cg9KvNhgWTdbmZsPo/messageFromApp";
const char* topicComOut = "Gsm/2h8Cg9KvNhgWTdbmZsPo/messageFromPlc";

bool REGISTERED = false;

// Inicializar Ethernet y cliente MQTT
EthernetClient ethClient;
PubSubClient mqtt(ethClient);

#define LED_PIN 13
#define LED_OPENING Q0_0
#define LED_CLOSING Q0_1

int ledStatus = LOW;
uint32_t lastReconnectAttempt = 0;
uint32_t lastPublishTime = 0; // Tiempo de la última publicación
const uint32_t publishInterval = 180000; // Intervalo de publicación (1 minuto)

unsigned long pulseStartTime = 0; // Almacenar el tiempo en que comenzó el pulso
bool isPulsing = false; // Estado de la señal PWM

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  payload[len] = '\0'; // Asegurar que el payload esté terminado en null
  Serial.println((char*)payload);

  // Proceder solo si el tema del mensaje entrante coincide
  if (String(topic) == String(topicComIn)) {
    int time = (SHORT_LON == true) ? 3000 : 5000;

    // Activación de motores y gestión de estados
    // Orden de abrir
    if (payload[0] == '1') {
      digitalWrite(LED_OPENING, HIGH);
      Serial.println("abriendose");
      //startPulses(Q0_6,frequencyStartOpen,precision);
      servo.writeMicroseconds(frequencyStartOpen);
      pulseStartTime = millis();
      isPulsing = true;
      mqtt.publish(topicComOut, "1");//abriendose
      //delay(time);
      //mqtt.publish(topicComOut, "4"); // Abierto
    } 
    // Orden de cerrar
    else if (payload[0] == '0') {
      digitalWrite(LED_CLOSING, HIGH);
      Serial.println("cerrandose");
      // digitalWrite(Q0_7, 300); // Activar motores
      // mqtt.publish(topicComOut, "2");//cerrandose
      //startPulses(Q0_6, frequencyStartClose, precision); // Iniciar el pulso para cerrar
      servo.writeMicroseconds(frequencyStartClockwise);
      pulseStartTime = millis(); // Registrar el tiempo de inicio
      isPulsing = true; // Indicar que el pulso está activo
      //mqtt.publish(topicComOut, "3");//cerrado
    } else {
      mqtt.publish(topicComOut, "0");
    }
  }
}

boolean mqttConnect() {
  Serial.print("Conectando a ");
  Serial.print(broker);
  // Conectarse al broker MQTT con autenticación
  boolean status = mqtt.connect("PLC", "PLC", "PLC");

  if (status == false) {
    Serial.println("Falló");
    return false;
  }

  Serial.println("Conectado");

  if (REGISTERED == false) {
   mqtt.publish(topicInit, ID_DEVICE);
    REGISTERED = true;
  }

  mqtt.subscribe(topicComIn);
  //mqtt.subscribe(topicComOut);
  // mqtt.subscribe(topicTemp);
  // mqtt.subscribe(topicHum);
  return mqtt.connected();
}

void setup() {
  // Configurar la velocidad de la consola
  Serial.begin(115200);
  pinMode(Q0_2, OUTPUT);
  pinMode(Q0_3, OUTPUT);
  pinMode(I0_10, INPUT);
  pinMode(I0_11, INPUT);
  //pinMode(Q0_6, OUTPUT);//salida motor
  
  servo.attach(Q0_6);
  servo.write(frequencyStop);
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

  // Configurar el broker MQTT
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);

  // Imprimir las rutas de los topics para comprobar
  Serial.println("Rutas de los topics:");
  Serial.print("topicInit: ");
  Serial.println(topicInit);
  Serial.print("topicHum: ");
  Serial.println(topicHum);
  Serial.print("topicTemp: ");
  Serial.println(topicTemp);
  Serial.print("topicComIn: ");
  Serial.println(topicComIn);
  Serial.print("topicComOut: ");
  Serial.println(topicComOut);
}

float readTemperature() {
  float value = analogRead(I0_11);
  float temperature = (value * (100.00 / 819.00) - (17720.00 / 273.00));
  Serial.println("Temperatura: " + String(temperature) + "ºC");
  return temperature;
}

float readHumidity() {
  float value = analogRead(I0_10);
  float humidity = ((value * (100.00 / 819.00)) - (6800.00 / 273.00))*-1;
  Serial.println("Humedad: " + String(humidity) + "%");
  return humidity;
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
    //startPulses(Q0_6, frequencyStop, precision); // Detener el pulso de apertura
    //stopPulses(Q0_6);
    isPulsing = false; // Indicar que el pulso ha terminado
    servo.write(frequencyStop);

    if (digitalRead(LED_OPENING) == HIGH) {
      digitalWrite(LED_OPENING, LOW);
      mqtt.publish(topicComOut, "4"); // Abierto
      Serial.println("Motor detenido después de 8 segundos (abierto)");
    }

    if (digitalRead(LED_CLOSING) == HIGH) {
      digitalWrite(LED_CLOSING, LOW);
      mqtt.publish(topicComOut, "3"); // Cerrado
      Serial.println("Motor detenido después de 8 segundos (cerrado)");
    }
  }

  // Publicar temperatura y humedad cada minuto
  uint32_t currentTime = millis();
  if (currentTime - lastPublishTime >= publishInterval) {
    lastPublishTime = currentTime;

    float temperature = readTemperature();
    char TempMsg[7];
    dtostrf(temperature, 6, 2, TempMsg);
    mqtt.publish(topicTemp, TempMsg);

    float humidity = readHumidity();
    char HumMsg[7];
    dtostrf(humidity, 6, 2, HumMsg);
    mqtt.publish(topicHum, HumMsg);
  }
}
