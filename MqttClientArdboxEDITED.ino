// Select your modem:
#define TINY_GSM_MODEM_SIM800

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

#include <SoftwareSerial.h>
SoftwareSerial SerialAT(14, 16);  // RX, TX del SIM800L en el Ardbox

//#define SerialAT Serial1

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
#define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN "8340"
#define MAX_TOPIC_LENGTH 25
const bool SHORT_LON = true;
const char ID_DEVICE[7] = "D_0001";

// Your GPRS credentials, if any
const char apn[]      = "internet.movil.es";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
//const char* broker = "test.mosquitto.org";
const char broker[20] = "test.mosquitto.org";
/*
const char topicHum[16]       = (String("Gsm/") + ID_DEVICE + String("/hum")).c_str();
const char topicInit[17]      = (String("Gsm/") + ID_DEVICE + String("/init")).c_str();
const char topicTemp[17]      = (String("Gsm/") + ID_DEVICE + String("/temp")).c_str();
const char topicComIn[18]     = (String("Gsm/") + ID_DEVICE + String("/comIn")).c_str();
const char topicComOut[19]    = (String("Gsm/") + ID_DEVICE + String("/comOut")).c_str();
*/

#define ID_DEVICE "D_0001"

#include <avr/pgmspace.h>

const char topicInit[] PROGMEM = "Gsm/init";
const char topicHum[] PROGMEM = "Gsm/" ID_DEVICE "/hum";
const char topicTemp[] PROGMEM = "Gsm/" ID_DEVICE "/temp";
const char topicComIn[] PROGMEM = "Gsm/" ID_DEVICE "/comIn";
const char topicComOut[] PROGMEM = "Gsm/" ID_DEVICE "/comOut";

bool REGISTERED = false;

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient  mqtt(client);

#define LED_PIN 13
#define LED_OPENING 11
#define LED_CLOSING 12

int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  //SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Only proceed if incoming message's topic matches
  if(String(topic) == topicComIn){
    int time = 0;

    if(SHORT_LON == true){
      time = 3000;
    }
    else{
      time = 5000;
    }

    // Activación de motores y gestión de estados
    if(payload == "1"){//Órden de abrir
      digitalWrite(LED_OPENING, HIGH); 
      mqtt.publish(topicComOut,4); //Abriendo
      delay(time);
      digitalWrite(LED_OPENING, LOW);
      mqtt.publish(topicComOut,1); //Abierto
    }
    else if(payload == "0"){ //Órden de cerrar
      digitalWrite(LED_CLOSING, HIGH); 
      mqtt.publish(topicComOut,2); //Cerrando
      delay(time); 
      digitalWrite(LED_CLOSING, LOW); 
      mqtt.publish(topicComOut,3); //Cerrado
    }
    else{
      mqtt.publish(topicComOut,99);
    }
  }
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);
  unsigned long last = 0;
  const unsigned long interval = 30000; 

  // Connect to MQTT Broker
  boolean status = mqtt.connect("GsmClientTest");

  if (status == false) {
    SerialMon.println("Failed");
    return false;
  }

  if(REGISTERED == false){
    mqtt.publish(topicInit, ID_DEVICE);
    mqtt.subscribe(topicComIn);
    REGISTERED = true;
  }
  else{
    unsigned long currentMillis = millis();
    if (currentMillis - last >= interval) {
      float temperature = readTemperature();
      char TempMsg[7];
      dtostrf(temperature, 6, 2, TempMsg);
      mqtt.publish(topicTemp, TempMsg);

      float hum = readHumidity();
      char HumMsg[7];
      dtostrf(hum, 6, 2, HumMsg);
      mqtt.publish(topicHum, TempMsg);
    }
  }
  return mqtt.connected();
}


void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  pinMode(I0_4, INPUT);
  pinMode(I0_3, INPUT);
  SerialMon.println("Starting");
  delay(10);

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_OPENING, OUTPUT);
  pinMode(LED_CLOSING, OUTPUT);

  SerialMon.println("Wait");

  // Set GSM module baud rate
  //TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  SerialAT.begin(115200);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.setBaud(38400);
  delay(100);
  SerialAT.end();
  SerialAT.begin(38400);
  modem.restart();
  //modem.init();


  String modemInfo = modem.getModemInfo();
  SerialMon.print("Info: ");
  SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif

#if TINY_GSM_USE_WIFI
  // Wifi connection parameters must be set before waiting for the network
  SerialMon.print(F("Setting password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" Failed");
    delay(10000);
    return;
  }
  SerialMon.println(" S");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS OK"); }
#endif

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
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
  humidity = (value*(100.00/819.00)-(6800.00/273.00));
  return humidity;
}

void loop() {
  if (!modem.isNetworkConnected()) {
    SerialMon.println("Network F");
    if (!modem.waitForNetwork(180000L, true)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    if (modem.isNetworkConnected()) {
      SerialMon.println("Re-connected");
    }

#if TINY_GSM_USE_GPRS
    // and make sure GPRS/EPS is still connected
    if (!modem.isGprsConnected()) {
      SerialMon.println("GPRS KO!");
      SerialMon.print(F("Connecting to "));
      SerialMon.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isGprsConnected()) { SerialMon.println("GPRS reconnected"); }
    }
#endif
  }

  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) { lastReconnectAttempt = 0; }
    }
    delay(100);
    return;
  }

  mqtt.loop();
}
