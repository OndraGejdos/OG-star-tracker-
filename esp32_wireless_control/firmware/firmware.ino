#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <string.h>
#include <esp_wifi.h>
#include <EEPROM.h>
#include "config.h"

// 3.7.2024
const int firmware_version = 2;

// Set your Wi-Fi credentials
const byte DNS_PORT = 53;
const char* ssid = "OG Star Tracker";  //change to your SSID
const char* password = "password123";    //change to your password, must be 8+ characters
//If you are using AP mode, you can access the website using the below URL
const String website_name = "www.tracker.com";
const int dither_intensity = 5;

const String tracking_on = "Tracking ON";
const String tracking_off = "Tracking OFF";

//Time b/w two rising edges should be 133.3333 ms
//66.666x2  ms
//sidereal rate = 0.00416 deg/s
//for 80Mhz APB (TIMER frequency)
#ifdef STEPPER_0_9
const uint64_t c_SIDEREAL_PERIOD = 2666666;
const uint32_t c_SLEW_SPEED = SLEW_SPEED;
const int arcsec_per_step = 2;
#else
const uint64_t c_SIDEREAL_PERIOD = 5333333;
const uint32_t c_SLEW_SPEED = SLEW_SPEED / 2;
const int arcsec_per_step = 4;
#endif

int slew_speed = 0, num_exp = 0, len_exp = 0, dither_on = 0, focal_length = 0, pixel_size = 0, steps_per_10pixels = 0, direction = c_DIRECTION;
float arcsec_per_pixel = 0.0;
unsigned long old_millis = 0, blink_millis = 0;
uint64_t exposure_delay = 0;

//state variables
bool s_slew_active = false, s_sidereal_active = false;  //change sidereal state to false if you want tracker to be OFF on power-up
enum interv_states { ACTIVE,
                     DELAY,
                     DITHER,
                     INACTIVE };
volatile enum interv_states interval_status = INACTIVE;

//2 bytes occupied by each int
//eeprom addresses
#define DITHER_ADDR 1
#define FOCAL_LEN_ADDR 3
#define PIXEL_SIZE_ADDR 5
#define DITHER_PIXELS 30  //how many pixels to dither

WebServer server(80);
DNSServer dnsServer;
hw_timer_t* timer0 = NULL;  //for sidereal rate
hw_timer_t* timer1 = NULL;  //for intervalometer control

void IRAM_ATTR timer0_ISR() {
  //sidereal ISR
  digitalWrite(AXIS1_STEP, !digitalRead(AXIS1_STEP));  //toggle step pin at required frequency
}

void IRAM_ATTR timer1_ISR() {
  //intervalometer ISR
  switch (interval_status) {
    case ACTIVE:
      num_exp--;
      if (num_exp == 0) {
        // no more images to capture, stop
        disableIntervalometer();
        num_exp = 0;
        len_exp = 0;
      } else if (num_exp % 3 == 0 && dither_on) {
        // user has active dithering and this is %3 image, stop capturing and run dither routine
        interval_status = DITHER;
        stopCapture();
        timerStop(timer1);  //pause the timer, wait for dither to finish in main loop
      } else {
        // run normally
        timerWrite(timer1, exposure_delay);
        stopCapture();
        interval_status = DELAY;
      }
      break;
    case DELAY:
      timerWrite(timer1, 0);
      startCapture();
      interval_status = ACTIVE;
      break;
  }
}

const String html =
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"    <title>OG Star Tracker Control Panel</title>\n"
"    <meta charset='utf-8'>\n"
"    <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
"    <style>\n"
"        body {\n"
"            background-color: #414141;\n"
"            text-align: center;\n"
"            font-family: \"Arial\";\n"
"            padding: 10px;\n"
"            display: flex;\n"
"            justify-content: center;\n"
"        }\n"
"\n"
"        button {\n"
"            background-color: #414141;\n"
"            color: white;\n"
"            border: none;\n"
"            padding: 10px 10px;\n"
"            text-align: center;\n"
"            text-decoration: none;\n"
"            display: inline-block;\n"
"            font-size: 12px;\n"
"            margin-top: 10px;\n"
"            cursor: pointer;\n"
"            width: 50%;\n"
"            min-width: 120px;\n"
"            font-weight: bold;\n"
"        }\n"
"\n"
"        select {\n"
"            font-size: 16px;\n"
"            padding: 5px;\n"
"            border: 2px solid #414141;\n"
"            border-radius: 0px;\n"
"        }\n"
"\n"
"        input[type='number'] {\n"
"            font-size: 16px;\n"
"            padding: 5px;\n"
"            border: 2px solid #414141;\n"
"            border-radius: 0px;\n"
"            box-sizing: border-box;\n"
"        }\n"
"\n"
"        label {\n"
"            display: inline-block;\n"
"            text-align: left;\n"
"            margin: 10px;\n"
"            font-size: 20px;\n"
"        }\n"
"\n"
"        h1 {\n"
"            font-size: 32px;\n"
"        }\n"
"\n"
"        h2 {\n"
"            font-size: 24px;\n"
"        }\n"
"\n"
"        h3 {\n"
"            font-size: 16px;\n"
"        }\n"
"\n"
"        summary {\n"
"            font-weight: bold;\n"
"            font-size: 16px;\n"
"            padding-top: 20px;\n"
"        }\n"
"\n"
"        .content {\n"
"            max-width: 450px;\n"
"            width: 100%;\n"
"            background-color: #d6d6d6;\n"
"            padding: 20px;\n"
"            border-radius: 5px;\n"
"        }\n"
"\n"
"        .card {\n"
"            background-color: #fafafa;\n"
"            padding: 10px 20px 20px 20px;\n"
"            border-radius: 5px;\n"
"            margin-top: 20px;\n"
"        }\n"
"\n"
"        .grid2 {\n"
"            display: flex;\n"
"            flex-direction: row;\n"
"            flex-wrap: wrap;\n"
"            justify-content: center;\n"
"            align-items: center;\n"
"        }\n"
"\n"
"        .grid2 > h3 {\n"
"            width: 50%;\n"
"            min-width: 120px;\n"
"        }\n"
"\n"
"        .grid2 > select,\n"
"        .grid2 > input {\n"
"            width: 50%;\n"
"            min-width: 120px;\n"
"            height: 40px;\n"
"        }\n"
"\n"
"        .grid2 > .checkbox {\n"
"            width: 50%;\n"
"            min-width: 120px;\n"
"            height: 20px;\n"
"            margin: 0px;\n"
"        }\n"
"\n"
"        .header {\n"
"            display: flex;\n"
"            justify-content: center;\n"
"        }\n"
"    \n"
"        #logo {\n"
"            background-image: url('data:image/png;base64,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');\n"
"            width: 220px;\n"
"            height: 176px;\n"
"            margin-top: 10px;\n"
"        }\n"
"\n"
"        .button-group {\n"
"            padding-top: 10px;\n"
"            display: flex;\n"
"            flex-wrap: wrap;\n"
"            justify-content: center;\n"
"        }\n"
"\n"
"        .right-separator {\n"
"            border-right: 2px solid #fafafa;\n"
"        }\n"
"\n"
"        .left-separator {\n"
"            border-left: 2px solid #fafafa;\n"
"        }\n"
"\n"
"        #status {\n"
"            font-size: 24px;\n"
"            margin: 0px;\n"
"        }\n"
"    </style>\n"
"    <script>\n"
"        function sendRequest(url) {\n"
"            var xhr = new XMLHttpRequest();\n"
"            xhr.onreadystatechange = function () {\n"
"                if (this.readyState == 4 && this.status == 200) {\n"
"                    document.getElementById('status').innerHTML = this.responseText;\n"
"                }\n"
"            };\n"
"            xhr.open('GET', url, true);\n"
"            xhr.send();\n"
"        }\n"
"\n"
"        setInterval(function () {\n"
"            sendRequest('/status');\n"
"        }, 20000);\n"
"\n"
"        function sendStartRequest(url) {\n"
"            var direction = document.getElementById('hemisphere-select').value;\n"
"            var starturl = url + '?direction=' + direction;\n"
"            sendRequest(starturl);\n"
"        }\n"
"\n"
"        function sendSlewRequest(url) {\n"
"            var speed = document.getElementById('slew-select').value;\n"
"            var slewurl = url + '?speed=' + speed;\n"
"            sendRequest(slewurl);\n"
"        }\n"
"\n"
"        function sendCaptureRequest() {\n"
"            var exposure = document.getElementById('exposure').value.trim();\n"
"            var numExposures = document.getElementById('num-exposures').value.trim();\n"
"            var focalLength = document.getElementById('focal_len').value.trim();\n"
"            var pixSize = Math.floor(parseFloat(document.getElementById('pixel_size').value.trim()) * 100);\n"
"\n"
"            var ditherEnabled = document.getElementById('dither_on').checked ? 1 : 0;\n"
"            var intervalometerUrl = '/start?exposure=' + exposure + '&numExposures=' + numExposures + '&focalLength=' + focalLength + '&pixSize=' + pixSize + '&ditherEnabled=' + ditherEnabled;\n"
"            sendRequest(intervalometerUrl);\n"
"        }\n"
"    </script>\n"
"</head>\n"
"\n"
"<body>\n"
"    <div class=\"content\">\n"
"        <div class=\"header\">\n"
"            <div id=\"logo\"></div>\n"
"        </div>\n"
"        <h1>Control Panel</h1>\n"
"        <div class=\"card\">\n"
"            <h2>Sidereal Tracking</h2>\n"
"            <div class=\"grid2\">\n"
"                <h3>Hemisphere:</h3>\n"
"                <select aria-label=\"Hemisphere\" id='hemisphere-select'>\n"
"                    <option value='1' %north%>North</option>\n"
"                    <option value='0' %south%>South</option>\n"
"                </select><br>\n"
"            </div>\n"
"            <div class=\"button-group\">\n"
"                <button class=\"right-separator\" type=\"button\" onclick=\"sendStartRequest('/on')\">ON</button>\n"
"                <button class=\"left-separator\" type=\"button\" onclick=\"sendRequest('/off')\">OFF</button>\n"
"            </div>\n"
"        </div>\n"
"        <div class=\"card\">\n"
"            <h2>Slew Control</h2>\n"
"            <div class=\"grid2\">\n"
"                <h3>Speed:</h3>\n"
"                <select aria-label=\"slew\" id='slew-select'>\n"
"                    <option value='1'>1 (Slowest)</option>\n"
"                    <option value='2'>2</option>\n"
"                    <option value='3'>3</option>\n"
"                    <option value='4'>4</option>\n"
"                    <option value='5'>5 (Quickest)</option>\n"
"                </select><br>\n"
"            </div>\n"
"            <div class=\"button-group\">\n"
"                <button class=\"right-separator\" type=\"button\" onclick=\"sendSlewRequest('/left')\">&#x2B9C;</button>\n"
"                <button class=\"left-separator\" type=\"button\" onclick=\"sendSlewRequest('/right')\">&#10148;</button>\n"
"            </div>\n"
"        </div>\n"
"        <div class=\"card\">\n"
"            <h2>Photo Control</h2>\n"
"            <div class=\"grid2\">\n"
"                <h3>Exposure lenght:</h3>\n"
"                <input type='number' id='exposure' placeholder='in seconds (Ex. 30)'>\n"
"            </div>\n"
"            <div class=\"grid2\">\n"
"                <h3>Number exposures:</h3>\n"
"                <input type='number' id='num-exposures' placeholder='nº of photos (Ex. 20)'>\n"
"            </div>\n"
"            <details>\n"
"                <summary>Dither Settings</summary>\n"
"                <!-- Content inside the collapsible section -->\n"
"                <div class=\"grid2\">\n"
"                    <h3>Dithering Enable:</h3>\n"
"                    <input class=\"checkbox\" aria-label=\"Dithering\" type=\"checkbox\" id=\"dither_on\" %dither%>\n"
"                </div>\n"
"                <div class=\"grid2\">\n"
"                    <h3>Lens focal lenght:</h3>\n"
"                    <input type=\"number\" id=\"focal_len\" placeholder='in milimiters (Ex. 135)' value='%focallen%'>\n"
"                </div>\n"
"                <div class=\"grid2\">\n"
"                    <h3>Camera pixel size:</h3>\n"
"                    <input type=\"number\" id=\"pixel_size\" placeholder='in micrometer (Ex. 4.1)' value='%pixsize%' step=\"0.01\">\n"
"                </div>\n"
"            </details>\n"
"            <div class=\"button-group\">\n"
"                <button class=\"right-separator\" type=\"button\" onclick=\"sendCaptureRequest()\">Start capture</button>\n"
"                <button class=\"left-separator\" type=\"button\" onclick=\"sendRequest('/abort')\">Abort capture</button>\n"
"            </div>\n"
"        </div>\n"
"        <div class=\"card\">\n"
"            <h2>Status</h2>\n"
"            <p id='status'>Test Message</p>\n"
"        </div>\n"
"    </div>\n"
"</body>\n"
"\n"
"</html>";

// Handle requests to the root URL ("/")
void handleRoot() {
  String formattedHtmlPage = String(html);
  formattedHtmlPage.replace("%north%", (direction ? "selected" : ""));
  formattedHtmlPage.replace("%south%", (!direction ? "selected" : ""));
  formattedHtmlPage.replace("%dither%", (dither_on ? "checked" : ""));
  formattedHtmlPage.replace("%focallen%", String(focal_length).c_str());
  formattedHtmlPage.replace("%pixsize%", String((float)pixel_size / 100, 2).c_str());
  server.send(200, "text/html", formattedHtmlPage);
}

void handleOn() {
  direction = server.arg("direction").toInt();
  s_sidereal_active = true;
  timerAlarmEnable(timer0);
  server.send(200, "text/plain", tracking_on);
}

void handleOff() {
  s_sidereal_active = false;
  timerAlarmDisable(timer0);
  server.send(200, "text/plain", tracking_off);
}

void handleLeft() {
  slew_speed = server.arg("speed").toInt();
  if (s_slew_active == false) {
    initSlew(0);
    s_slew_active = true;
  }
  old_millis = millis();
  server.send(200, "text/plain", "Slewing");
}

void handleRight() {
  slew_speed = server.arg("speed").toInt();
  old_millis = millis();
  if (s_slew_active == false) {
    initSlew(1);  //reverse direction
    s_slew_active = true;
  }
  server.send(200, "text/plain", "Slewing");
}

void handleStartCapture() {
  if (interval_status == INACTIVE) {
    len_exp = server.arg("exposure").toInt();
    num_exp = server.arg("numExposures").toInt();
    dither_on = server.arg("ditherEnabled").toInt();
    focal_length = server.arg("focalLength").toInt();
    pixel_size = server.arg("pixSize").toInt();

    if ((len_exp == 0 || num_exp == 0)) {
      server.send(200, "text/plain", "Invalid Intervalometer Settings!");
      return;
    } else if (dither_on && (focal_length == 0 || pixel_size == 0)) {
      server.send(200, "text/plain", "Invalid Dither Settings!");
      return;
    }
    updateEEPROM(dither_on, focal_length, pixel_size);
    arcsec_per_pixel = (((float)pixel_size / 100.0) / focal_length) * 206.265;        //div pixel size by 100 since we multiplied it by 100 in html page
    steps_per_10pixels = (int)(((arcsec_per_pixel * 10.0) / arcsec_per_step) + 0.5);  //add 0.5 to round up float to nearest int while casting
    Serial.println("steps per 10px: ");
    Serial.println(steps_per_10pixels);

    interval_status = ACTIVE;
    exposure_delay = ((len_exp - 3) * 2000);  // 3 sec delay
    initIntervalometer();
    server.send(200, "text/plain", "Capture ON");
  } else {
    server.send(200, "text/plain", "Capture Already ON");
  }
}

void handleAbortCapture() {
  if (interval_status == INACTIVE) {
    server.send(200, "text/plain", "Capture Already OFF");
  } else {
    disableIntervalometer();
    num_exp = 0;
    len_exp = 0;
    interval_status = INACTIVE;
    server.send(200, "text/plain", "Capture OFF");
  }
}

void handleStatusRequest() {
  if (interval_status != INACTIVE) {
    char status[60];
    sprintf(status, "%d Captures Remaining...", num_exp);
    server.send(200, "text/plain", status);
  } else if (s_sidereal_active) {
    server.send(200, "text/plain", tracking_on);
  } else {
    server.send(204, "text/plain", "dummy");
  }

  // TODO add detection for capturing
}

void handleVersion() {
  server.send(200, "text/plain", (String)firmware_version);
}

void writeEEPROM(int address, int value) {
  byte high = value >> 8;
  byte low = value & 0xFF;
  EEPROM.write(address, high);
  EEPROM.write(address + 1, low);
}

int readEEPROM(int address) {
  byte high = EEPROM.read(address);
  byte low = EEPROM.read(address + 1);
  return ((high << 8) + low);
}

void updateEEPROM(int dither, int focal_len, int pix_size) {
  if (readEEPROM(DITHER_ADDR) != dither) {
    writeEEPROM(DITHER_ADDR, dither);
    //Serial.println("dither updated");
  }
  if (readEEPROM(FOCAL_LEN_ADDR) != focal_len) {
    writeEEPROM(FOCAL_LEN_ADDR, focal_len);
    //Serial.println("focal length updated");
  }
  if (readEEPROM(PIXEL_SIZE_ADDR) != pix_size) {
    writeEEPROM(PIXEL_SIZE_ADDR, pix_size);
    //Serial.println("pix size updated");
  }
  EEPROM.commit();
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
  digitalWrite(AXIS1_DIR, direction);
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
  timerAlarmWrite(timer1, (len_exp * 2000), true);  //2000 because prescaler cant be more than 16bit, = 1sec ISR freq
  timerAlarmEnable(timer1);
  startCapture();
}
void disableIntervalometer() {
  stopCapture();
  timerAlarmDisable(timer1);
  timerDetachInterrupt(timer1);
  timerEnd(timer1);
}

void stopCapture() {
  digitalWrite(INTERV_PIN, LOW);
}

void startCapture() {
  digitalWrite(INTERV_PIN, HIGH);
}

void ditherRoutine() {
  int i = 0, j = 0;
  timerAlarmDisable(timer0);
  digitalWrite(AXIS1_DIR, random(2));  //dither in a random direction
  delay(500);
  Serial.println("Dither rndm direction:");
  Serial.println(random(2));

  for (i = 0; i < dither_intensity; i++) {
    for (j = 0; j < steps_per_10pixels; j++) {
      digitalWrite(AXIS1_STEP, !digitalRead(AXIS1_STEP));
      delay(10);
      digitalWrite(AXIS1_STEP, !digitalRead(AXIS1_STEP));
      delay(10);
    }
  }
  delay(1000);
  initSiderealTracking();
  delay(3000);  //settling time after dither
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);  //SIZE = 6 bytes, 2 bytes for each variable
  //fetch values from EEPROM
  dither_on = readEEPROM(DITHER_ADDR);
  focal_length = readEEPROM(FOCAL_LEN_ADDR);
  pixel_size = readEEPROM(PIXEL_SIZE_ADDR);

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
  server.on("/version", HTTP_GET, handleVersion);

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
  if (interval_status == DITHER) {
    disableIntervalometer();
    ditherRoutine();
    interval_status = ACTIVE;
    initIntervalometer();
  }
  server.handleClient();
  dnsServer.processNextRequest();
}