#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <string.h>
#include <esp_wifi.h>
#include "config.h"

// Set your Wi-Fi credentials
const byte DNS_PORT = 53;
const char* ssid = "OGST_V2";      //change to your SSID
const char* password = "12345678";        //change to your password, must be 8+ characters
//If you are using AP mode, you can access the website using the below URL
const String website_name = "www.OGST.com";

//Time b/w two rising edges should be 133.3333 ms
//66.666x2  ms
//sidereal rate = 0.00416 deg/s
#ifdef STEPPER_0_9
const uint64_t c_SIDEREAL_PERIOD = 2666666;
#else
const uint64_t c_SIDEREAL_PERIOD = 5333333;
c_SLEW_SPEED = c_SLEW_SPEED / 2;
#endif

int slew_speed = 0, num_exp = 0, len_exp = 0;
unsigned long old_millis = 0, blink_millis = 0;
uint64_t exposure_delay = 0;

//state variables
bool s_slew_active = false, s_sidereal_active = true;  //change sidereal state to false if you want tracker to be OFF on power-up
enum interv_states { ACTIVE,
                     DELAY,
                     INACTIVE };
enum interv_states s_interv = INACTIVE;

WebServer server(80);
DNSServer dnsServer;
hw_timer_t* timer0 = NULL;  //for sidereal rate
hw_timer_t* timer1 = NULL;  //for intervalometer control

void IRAM_ATTR timer0_ISR() {
  //sidereal ISR
  digitalWrite(AXIS1_STEP, !digitalRead(AXIS1_STEP));  //toggle step pin
}

void IRAM_ATTR timer1_ISR() {
  //intervalometer ISR
  if (s_interv == DELAY) {
    timerWrite(timer1, 0);
    digitalWrite(INTERV_PIN, HIGH);
    s_interv = ACTIVE;
  } else if (s_interv == ACTIVE) {
    num_exp--;
    if (num_exp == 0) {
      disableIntervalometer();
    } else {
      timerWrite(timer1, exposure_delay);
      digitalWrite(INTERV_PIN, LOW);
      s_interv = DELAY;
    }
  }
}

// Handle requests to the root URL ("/")
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>OG Star Tracker Control</title><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{background-color: lightcoral; text-align: center;}button{background-color: white; color: black; border: none; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer;}select{font-size: 16px; padding: 5px;}input[type='number']{font-size: 16px; padding: 5px; width: 50%;}label{display: inline-block; text-align: left; margin: 10px; font-size: 20px;}#status{font-size: 24px; margin: 20px;}</style><script>function sendRequest(url) {var xhr = new XMLHttpRequest();xhr.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {document.getElementById('status').innerHTML = this.responseText;}};xhr.open('GET', url, true);xhr.send();}setInterval(function(){sendRequest('/status');}, 20000);function sendSlewRequest(url) {var speed = document.getElementById('slew-select').value;var slewurl = url + '?speed=' + speed;sendRequest(slewurl);}function sendCaptureRequest() {var exposure = document.getElementById('exposure').value.trim();var numExposures = document.getElementById('num-exposures').value.trim();var intervalometerUrl = '/start?exposure=' + exposure + '&numExposures=' + numExposures;sendRequest(intervalometerUrl);} </script></head><body><h1>OG Star Tracker Control</h1><label>Sidereal Tracking:</label><br><button onclick=\"sendRequest('/on')\">ON</button><button onclick=\"sendRequest('/off')\">OFF</button><br><label>Slew Control:</label><br><label>Speed:</label><select id='slew-select'><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option><option value='4'>4</option><option value='5'>5</option></select><br><button onclick=\"sendSlewRequest('/left')\">&#8592;</button><button onclick=\"sendSlewRequest('/right')\">&#8594;</button><br><label>Intervalometer Control:</label><br><input type='number' id='exposure' placeholder='Exposure length (s)'><input type='number' id='num-exposures' placeholder='Number of Exposures'><br><button onclick=\"sendCaptureRequest()\">Start capture</button><button onclick=\"sendRequest('/abort')\">Abort capture</button><br><label>STATUS:</label><br><p id='status'></p></body></html>";
  server.send(200, "text/html", html);
}


void handleOn() {
  s_sidereal_active = true;
  timerAlarmEnable(timer0);
  server.send(200, "text/plain", "Tracking ON");
}

void handleOff() {
  s_sidereal_active = false;
  timerAlarmDisable(timer0);
  server.send(200, "text/plain", "Tracking OFF");
}

void handleLeft() {
  slew_speed = server.arg("speed").toInt();
  if (s_slew_active == false) {
    initSlew(c_DIRECTION);
    s_slew_active = true;
  }
  old_millis = millis();
  server.send(200, "text/plain", "Slewing");
}

void handleRight() {
  slew_speed = server.arg("speed").toInt();
  old_millis = millis();
  if (s_slew_active == false) {
    initSlew(!c_DIRECTION);  //reverse direction
    s_slew_active = true;
  }
  server.send(200, "text/plain", "Slewing");
}

void handleStartCapture() {
  if (s_interv == INACTIVE) {
    len_exp = server.arg("exposure").toInt();
    num_exp = server.arg("numExposures").toInt();
    if (len_exp == 0 || num_exp == 0) {
      server.send(200, "text/plain", "Invalid Settings");
      return;
    }
    s_interv = ACTIVE;
    exposure_delay = ((len_exp - 3) * 2000);  // 3 sec delay
    initIntervalometer();
    server.send(200, "text/plain", "Capture ON");
  } else {
    server.send(200, "text/plain", "Capture Already ON");
  }
}

void handleAbortCapture() {
  if (s_interv == INACTIVE) {
    server.send(200, "text/plain", "Capture Already OFF");
  } else {
    disableIntervalometer();
    server.send(200, "text/plain", "Capture OFF");
  }
}

void handleStatusRequest() {
  if (s_interv != INACTIVE) {
    char status[60];
    sprintf(status, "%d Captures Remaining...", num_exp);
    server.send(200, "text/plain", status);
  } else
    server.send(204, "text/plain", "dummy");
}

void setMicrostep(int microstep) {
  switch (microstep) {
    case 8:
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, LOW);
      break;
    case 16:
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, HIGH);
      break;
    case 32:
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, LOW);
      break;
    case 64:
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, HIGH);
      break;
  }
}
void initSlew(int dir) {
  timerAlarmDisable(timer0);
  digitalWrite(AXIS1_DIR, dir);
  setMicrostep(8);
  ledcSetup(0, (c_SLEW_SPEED * slew_speed), 8);
  ledcAttachPin(AXIS1_STEP, 0);
  ledcWrite(0, 127);  //50% duty pwm
}
void initSiderealTracking() {
  digitalWrite(AXIS1_DIR, c_DIRECTION);
  setMicrostep(16);
  timerAlarmWrite(timer0, c_SIDEREAL_PERIOD, true);
  if (s_sidereal_active == true)
    timerAlarmEnable(timer0);
  else
    timerAlarmDisable(timer0);
}
void initIntervalometer() {
  timer1 = timerBegin(1, 40000, true);
  timerAttachInterrupt(timer1, &timer1_ISR, true);
  timerAlarmWrite(timer1, (len_exp * 2000), true);
  timerAlarmEnable(timer1);
  digitalWrite(INTERV_PIN, HIGH);  // start the first capture
}
void disableIntervalometer() {
  digitalWrite(INTERV_PIN, LOW);
  timerAlarmDisable(timer1);
  timerDetachInterrupt(timer1);
  timerEnd(timer1);
  num_exp = 0;
  len_exp = 0;
  s_interv = INACTIVE;
}
void setup() {

  Serial.begin(115200);

#ifdef AP
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(ssid, password);
  delay(500);
  Serial.println("Creating Wifi Network");

  //ANDROID 10 WORKAROUND==================================================
  //set new WiFi configurations
  WiFi.disconnect();
  Serial.println("reconfig WiFi...");
  /*Stop wifi to change config parameters*/
  esp_wifi_stop();
  esp_wifi_deinit();
  /*Disabling AMPDU RX is necessary for Android 10 support*/
  wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();  //We use the default config ...
  my_config.ampdu_rx_enable = 0;                              //... and modify only what we want.
  Serial.println("WiFi: Disabled AMPDU...");
  esp_wifi_init(&my_config);  //set the new config = "Disable AMPDU"
  esp_wifi_start();           //Restart WiFi
  delay(500);
  //ANDROID 10 WORKAROUND==================================================
#else
  WiFi.mode(WIFI_MODE_STA);  // Set ESP32 in station mode
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Network in STA mode");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
#endif
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, website_name, WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleOn);
  server.on("/off", HTTP_GET, handleOff);
  server.on("/left", HTTP_GET, handleLeft);
  server.on("/right", HTTP_GET, handleRight);
  server.on("/start", HTTP_GET, handleStartCapture);
  server.on("/abort", HTTP_GET, handleAbortCapture);
  server.on("/status", HTTP_GET, handleStatusRequest);

  // Start the server
  server.begin();

#ifdef AP
  Serial.println(WiFi.softAPIP());
#else
  Serial.println(WiFi.localIP());
#endif
  pinMode(INTERV_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(AXIS1_STEP, OUTPUT);
  pinMode(AXIS1_DIR, OUTPUT);
  pinMode(EN12_n, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  digitalWrite(AXIS1_STEP, LOW);
  digitalWrite(EN12_n, LOW);

  timer0 = timerBegin(0, 2, true);
  timerAttachInterrupt(timer0, &timer0_ISR, true);
  initSiderealTracking();
}

void loop() {
  if (s_slew_active) {
    //blink status led if mount is in slew mode
    if (millis() - blink_millis >= 150) {
      digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
      blink_millis = millis();
    }
  } else {
    //turn on status led if sidereal tracking is ON
    digitalWrite(STATUS_LED, (s_sidereal_active == true));
  }
  if ((s_slew_active == true) && (millis() - old_millis >= 1200)) {
    //slewing will stop if button is not pressed again within 1.2sec
    s_slew_active = false;
    ledcDetachPin(AXIS1_STEP);
    pinMode(AXIS1_STEP, OUTPUT);
    initSiderealTracking();
  }

  server.handleClient();
  dnsServer.processNextRequest();
}
