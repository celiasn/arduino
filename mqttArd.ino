#include <PubSubClient.h>
#include <GPRS.h>
#include <ArduinoJson.h>

#define PIN "8340"
#define APN "internet.movil.es"
#define GPRS_USERNAME ""
#define GPRS_PASSWORD ""
#define MQTT_SERVER "" //Dirección de tu servidor MQTT
#define MQTT_PORT 1883
#define MQTT_TOPIC_TEMPERATURE "Project/PLC/temperature"
#define MQTT_TOPIC_HUMIDITY "Project/PLC/humidity"
#define MQTT_TOPIC_CONTROL1 "Project/app/open"
#define MQTT_TOPIC_CONTROL2 "Project/app/close"
#define M6_TIME ""
#define M9_TIME ""

IPAddress mqtt_server(192,168,1,42); //LAN Inalámbrica de wifi (IPv4 de mi ordenador)
GPRSClient gprsClient;
PubSubClient mqttClient(gprsClient);

const char* mqttClientId = "PLCClient";

void setup() {
  Serial.begin(9600);
  pinMode(I0_4, INPUT);
  pinMode(I0_3, INPUT);

  //Unlock SIM
  if(!GPRS.begin()){
    Serial.println("Impossible to begin GPRS");
    while(true);
  }
  int pinRequired = GPRS.isPINRequired();
  if (pinRequired == 1) {
    if (!GPRS.unlockSIM(PIN)) {
      Serial.println("Invalid PIN");
      while (true);
    }
  }
  else if (pinRequired != 0) {
    Serial.println("Blocked SIM");
    while (true);
  }
  
  //MQTT Client configuration
  mqttClient.setServer("192.168.1.42", MQTT_PORT);
  mqttClient.setCallback(callback);

  //Connecting GPRS
  connectGPRS();

  //MQTT Connection
  connectMQTT();
}

void loop() {
  // Verificar si la conexión MQTT está activa
  if (!mqttClient.connected()) {
    connectMQTT(); // Reintentar la conexión MQTT si es necesario
  }

  // Simular la lectura de temperatura y humedad
  float temperature = readTemperature();
  float humidity = readHumidity();

  // Enviar datos al servidor MQTT
  publishSensorData(temperature, humidity);

  // Manejar las solicitudes de control MQTT
  mqttClient.loop();
}

void connectGPRS() {
  // Configurar y conectar al servicio GPRS
  if (!GPRS.enableGPRS(APN, GPRS_USERNAME, GPRS_PASSWORD)) {
    Serial.println("Error al habilitar GPRS");
    //while (true);
  }
  Serial.println("GPRS habilitado");
}

void connectMQTT() {
  // Conectar al servidor MQTT
  if (mqttClient.connect(mqttClientId)) {
    Serial.println("Conectado al servidor MQTT");
    // Suscribirse al tema de control
    mqttClient.subscribe(MQTT_TOPIC_CONTROL1);
    mqttClient.subscribe(MQTT_TOPIC_CONTROL2);
  } else {
    Serial.print("Error al conectar al servidor MQTT, rc=");
    Serial.println(mqttClient.state());
  }
}

float readTemperature(){
  float value = analogRead(I0_3);
  float temperature = 0.00;
  temperature= (value*(100.00/819.00)- (17720.00/273.00));
  return temperature;
}

float readHumidity(){
  float value = analogRead(I0_4); 
  float humidity = 0.0;
  humidity = (value*10/1023)-2;
  return humidity*10;
}

void publishSensorData(float temperature, float humidity) {
  // Crear un objeto JSON para los datos
  StaticJsonDocument<128> jsonDoc;
  jsonDoc["temperature"] = temperature;
  jsonDoc["humidity"] = humidity;

  // Convertir el objeto JSON en una cadena
  String payload;
  serializeJson(jsonDoc, payload);

  // Publicar los datos en los temas MQTT correspondientes
  mqttClient.publish(MQTT_TOPIC_TEMPERATURE, payload.c_str());
  mqttClient.publish(MQTT_TOPIC_HUMIDITY, payload.c_str());
}

// Esta función maneja los mensajes recibidos desde el servidor MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message: ");
  Serial.println(topic);

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  float time = 0.0;
  if(message.equals("6_metros")){
    time = 6;
  }
  else if(message.equals("9_metros")){
    time = 9;
  }
  else{
    //Lanzar error
  }

  //ABRIR
  if (strcmp(topic, MQTT_TOPIC_CONTROL1) == 0) {
    //Comprueba la validez del mensaje
    if(time != 0){
      //Activa los motores, abriendo durante el tiempo correspondiente
    }
  }

  //CERRAR
  if(strcmp(topic, MQTT_TOPIC_CONTROL2) == 0){
    //Comprueba la validez del mensaje
    if(time != 0){
      //Activar los motores, cerrando durante el tiempo correspondiente
    }
  }
}
