#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#define dir 13
#define step 14
int number = 0;
int length = 0;
int move = 1 ;
const char* ssid = "ssid";
const char* password = "pasword";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
StaticJsonDocument<200> send;
StaticJsonDocument<200> rec;
unsigned long startMillis;
unsigned long currentMillis;
bool direction  = LOW;
bool newRequest = false;
bool hemisphereNS = false;
bool ismovingright = false;
bool ismovingleft = false;
int move_speed = 0;
bool moveEnd = false;
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void sendResponse( AsyncWebSocketClient* client) {
  // Create a JSON message to send as response
  StaticJsonDocument<200> response;
  response["response"] = "OK";
  String responseJson;
  serializeJson(response, responseJson);
  
  // Send the response message to the client
  client->text(responseJson);
}
int trackspeed = 1;
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    // Parse the incoming JSON message
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    else {
      number = doc["exposures"];
      length = doc["length"];
      move  = doc["speed"];
      if (doc.containsKey("hemisphereNS") == true ) {
        Serial.println("Received a hemisphere command");
        hemisphereNS  = doc["hemisphereNS"];
        if (hemisphereNS == true){
          direction = HIGH;
        }
        if (hemisphereNS == false){
          direction = LOW;
        }
      }
      if (number !=0) {
        number = doc["exposures"];
        length = doc["length"];
        Serial.print(F("Received camera message: exposures= "));
        Serial.print(number);
        Serial.print(F(", length= "));
        Serial.print(length);
        Serial.println("");
      }
      if (doc.containsKey("speed") ==true ) {
        Serial.println("Received a custom speed comand");
        trackspeed  = doc["speed"];
      }
      if (doc.containsKey("slewe") == true ){
        move_speed = doc["slewe"] ;
        Serial.println("Received a slew speed comand");
      }
      if (doc.containsKey("righton") == true ){
        ismovingright = true;
        moveEnd = true;
        Serial.println("Received a right move comand");
      }
      if (doc.containsKey("lefton") == true ){
        Serial.println("Received a left move comand");
        moveEnd = true;
        ismovingleft = true;
      }
      if (doc.containsKey("meybe") == true ){
        Serial.println("move command end");
        moveEnd = false;
        ismovingleft = false ;
        ismovingleft = false ;
      }
      // Handle the JSON message here
      sendResponse(client);
    }
  }
}
int trackingrate = 65; // sideral speed
void setup() {
  Serial.begin(115200);
  initWiFi();
  initSPIFFS();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/web.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}
void loop() {
  move = 1*trackspeed;
  currentMillis = millis();
  if ((ismovingright == true)||(ismovingleft == true)){
    Serial.println("Doing a move comand");
    move = move*move_speed*0.0001;
    if (ismovingright = true){
       digitalWrite(dir,HIGH);
    }
    if (ismovingleft = true){
       digitalWrite(dir,LOW);
    }
  }
  if (currentMillis - startMillis >= trackingrate*move)  
  {
    digitalWrite(step,HIGH);  
    startMillis = currentMillis;  
    int printos = trackingrate*move ;
    Serial.println(printos);
  }
  currentMillis = millis();
  if (currentMillis - startMillis >= trackingrate*move)  
  {
    digitalWrite(step,LOW);  
    startMillis = currentMillis;  
  }
  if (moveEnd == false){
    move = 1*trackingrate;
  }
}


