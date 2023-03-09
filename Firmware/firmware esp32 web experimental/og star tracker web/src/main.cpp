#include <Arduino.h> 
#include <WebSocketsServer.h>    
#include <WiFi.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WifiAP.h>
#define dir 13 
#define sptep 14 
const char* ssid = "ssid";
const char* password = "#pasword";
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); 
StaticJsonDocument<200> send;
StaticJsonDocument<200> rec;
bool newRequest = false;
void initSPIFFS() { //  initializes the ESP32 Filesystem
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
    Serial.println("SPIFFS mounted successfully");
  }
}
void initWiFi() { //function initializes WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      StaticJsonDocument<200> rec;                    // create a JSON container
      DeserializationError error = deserializeJson(rec, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // JSON string was received correctly, so information can be retrieved:
        int Exposure = rec["exposures"];
        int Lenght = rec["lenght"];
        Serial.println(Lenght);
        Serial.println(Exposure);
      }
  break;
  }
}
void setup() {
  Serial.begin(115200);
  initWiFi();
  initSPIFFS();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/web.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();                                   // Update function for the webSockets 
}
