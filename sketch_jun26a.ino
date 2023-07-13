// GPRS library example
// by Industrial Shields

#include <GPRS.h>
//#include <ArduinoJson.h>
//#include <HTTPClient.h>

#define PIN "8340"
#define APN "internet.movil.es"
#define USERNAME ""
#define PASSWORD ""

GPRSClient client;
//uint8_t buffer[1024];
uint8_t buffer[512];

////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600UL);

  if (!GPRS.begin()) {
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  //Serial.println("Entering the loop");
  static uint32_t lastStatusTime = millis();
  if (millis() - lastStatusTime > 2000) {
    uint8_t networkStatus = GPRS.getNetworkStatus();
    Serial.print("Status: ");
    Serial.println(networkStatus);
    lastStatusTime = millis();

    if ((networkStatus == 1) || (networkStatus == 5)) {
      int GPRSStatus = GPRS.getGPRSStatus();
      if (GPRSStatus == 0) {
        if (!GPRS.enableGPRS(APN, USERNAME, PASSWORD)) {
          Serial.println("GPRS not enabled");
        }
      }
      else if (GPRSStatus == 1) {
        if (GPRS.connected()) {
          if (!client.connected()) {
            static bool requestDone = false;
            if (!requestDone) {
              if (!client.connect("www.industrialshields.com", 80)) { //"www.industrialshields.com", 80  //"http://localhost", 3000
                Serial.println("Error connecting to web");
              }
              else {
                client.println("GET /index.html HTTP/1.1");
                client.println("Host: www.industrialshields.com");
                client.println("User-Agent: GPRS-PLC");
                client.println();
                Serial.println("Connected");
                Serial.println("Data sent");
                requestDone = true;
              }
            }
          }
          else if (client.available()) {
            Serial.println("HTTP response:");
            Serial.write(buffer, client.read(buffer, sizeof(buffer)));
            client.stop();
          }
        }
      }
    }
  }
}
