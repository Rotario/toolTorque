#include <genieArduino.h>
#include "HX711.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include "chuckParser.h"
#define DEBUG false
#define Serial1 if(DEBUG)Serial1

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                                    // a File variable to temporarily store the received file

const char *ssid = "NIKKEN-WAPE"; // The name of the Wi-Fi network that will be created
const char *password = "2080774004";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

const char* mdnsName = "esp8266"; // Domain name for the mDNS responder

Genie genie;
#define RESETLINE 5  // Change this if you are not using an Arduino Adaptor Shield Version 2 (see code below)

#define BUZZER 4
#define BUZZER_PWM 1 //Is the buzzer a 
#if BUZZER_PWM
uint8_t calledPWM = 0;
#endif

HX711 scale;
#define DOUT  D5
#define CLK  D6

int measuredTorque = 0;
int previousTorque = 0;

float scaleFactor = 10.00;
float gaugeMax = 100.00;

float calibration_factor;
int model = 0;
int size = 0;
int shank = 0;

uint8_t enBuzzer;

uint8_t holdOn = 0;

#define AUTH_BUF_SIZE 5 //4 digit + null
uint8_t authBufidx = 0;
char authBuf[AUTH_BUF_SIZE];


Chuck modelArr[NO_MODELS];

const char * models[] = {"Slim-Chuck/Major Dream", "Victory-Chuck"};
const int noModels = sizeof(models) / sizeof(models[0]);

const char * chucks[2][6] = {
  {"SK6", "SK10", "SK13", "SK16", "SK20", "SK25"},
  {"VCK6", "VCK13"}
};
const int noChucks[] = {6, 2};

const char * shanks[3][4] = {
  {"<3mm", "4-6mm", "8-10mm", ">12mm"},//SK
  {"<3mm", "3-6mm"},  //VCK6
  {"<5mm", "5-12mm"} //VCK13
};

const int noShanks[2][6] = {{2, 3, 4, 4, 4, 4}, {2, 2}};

const int TorqueSetpoint[2][6][4] = {
  { //SK
    {20, 30}, //SK6
    {25, 40, 45}, //SK10
    {25, 40, 50, 50}, //SK13
    {50, 50, 65, 75}, //SK16
    {70, 70, 70, 90},  //SK20
    {85, 85, 85, 100}//SK25
  },
  { //VCK
    {30, 50}, //VCK16
    {60, 75} //VCK13
  }
};

#include "defines.h"

void setup()
{

  Serial.begin(256000);
  EEPROM.begin(12);//EEPROM for cal factor https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/examples/eeprom_write/eeprom_write.ino
  //Serial1.swap();

  genie.Begin(Serial);   // Use Serial10 for talking to the Genie Library, and to the 4D Systems display
  genie.AttachEventHandler(myGenieEventHandler); // Attach the user function Event Handler for processing events

  pinMode(BUZZER, OUTPUT); //Buzzer


  pinMode(RESETLINE, OUTPUT);  // Set D4 on Arduino to Output
  digitalWrite(RESETLINE, LOW);  // Reset the Display via D4
  delay(100);
  digitalWrite(RESETLINE, HIGH);  // unReset the Display via D4

  delay (3500); //let the display start up after the reset (This is important)

  changeForm(F_TORQUE);

  Serial1.begin(115200);  // Serial10 @ 200000 (200K) Baud

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

  startOTA();                  // Start the OTA service

  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server

  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler

  // Add rest variables
  //TODO: void pointer, doesn't give correct value, fix!
  addRestVariable("torque", &measuredTorque);
  //addRestVariable("calfactor", &calibration_factor);

  File f = SPIFFS.open("/chucks.txt", "r");
  if (f && readChuckFile(modelArr, &f) == 0) {
  } else {
    f = SPIFFS.open("/chucks.txt.bak", "r");
    if (f && readChuckFile(modelArr, &f) == 0) {
    } else {
      genie.WriteStr(STR_MODEL, "Config File Read FAILED!");
      genie.WriteStr(STR_CHUCK, "Contact NIKKEN Service");
      genie.WriteStr(STR_SHANK, "01709 366306");
      while(1);
    }
  }

  f.close();

  updateScreenTorqueModels();

  //pinMode(13, OUTPUT); //LED
  //pinMode(23, OUTPUT); //Power for load cell
  //digitalWrite(23, HIGH); //Turn on load cell
  Serial1.println("Copyright NIKKEN Kosakusho Europe Ltd. 2019");
  Serial1.println("Welcome!");

  EEPROM.get(ADDR_CAL, calibration_factor);
  EEPROM.get(ADDR_ENBUZZER, enBuzzer);
  Serial1.print("Calibration Factor= ");
  Serial1.println(calibration_factor);
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial1.println("To calibrate, enter C:");
  scaleFactor = gaugeMax / modelArr[model].torques[size][shank];
  analogWriteFreq(2000); //Set PWM freq for buzzer

  //calibrateScale();

}
int test = 1;
void loop()
{

  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events
  genie.DoEvents();// This calls the library each loop to process the queued responses from the display



  if (scale.is_ready() && screen.form == F_TORQUE || screen.form == F_SETTINGS) {
    measureTorque();
  }

}

void measureTorque() {
  measuredTorque = scale.get_units();
  //Scale Torque to bar 0-100
  float scaledTorque = scaleFactor * measuredTorque;
  if (scaledTorque >= 100) {
    scaledTorque = 100;
  }
  if (scaledTorque < 0) {
    scaledTorque = 0;
  }

#if BUZZER_PWM
  if (enBuzzer && scaledTorque > 90) { //95% of setpoint
    if (!calledPWM) {
      calledPWM = 1;
      analogWrite(BUZZER, 511);//50% duty cycle
    }
  } else {
    calledPWM = 0;
    digitalWrite(BUZZER, LOW);//50% duty cycle
  }
#else
  if (enBuzzer && scaledTorque > 90) { //95% of setpoint
    digitalWrite(BUZZER, HIGH);
  } else {
    digitalWrite(BUZZER, LOW);
  }
#endif

  //Send if either hold is off, or hold is on and this torque is larger than previous
  if (holdOn == 1 && measuredTorque > previousTorque || holdOn == 0) {
    updateTorqueScreen(measuredTorque, scaledTorque);
    updateWebTorque(measuredTorque);
  }

  //holdAdmin
  if (holdOn == 1 && measuredTorque > previousTorque) {
    previousTorque = measuredTorque;
  } else if (holdOn == 0) {
    previousTorque = 0;
  }
}

float calibrateScale(void) {
  char buf[100];
  char remBuf[25] = "Remove all torque";
  char appBuf[25] = "Apply 20Nm of torque";
  int s = sizeof(remBuf);
  genie.WriteStr(STR_CALDIALOG, remBuf);
  for (int i = 0; i < 3; i++) {
    strncat(remBuf, ".", s);
    genie.WriteStr(STR_CALDIALOG, remBuf);
    delay(1000);
  }

  scale.set_scale(1);//Sets scale back to 1
  scale.tare(5); //Takes 3 tare vals
  genie.WriteStr(STR_CALDIALOG, appBuf);
  for (int i = 0; i < 3; i++) {
    strncat(appBuf, ".", s);
    genie.WriteStr(STR_CALDIALOG, appBuf);
    delay(1000);
  }

  float measured = scale.get_value(5); //This is tared value divided by 1, as scale set back to 1
  float prevCal = calibration_factor;
  calibration_factor = measured / 20;

  snprintf(buf, s, "CalF: %.2f", calibration_factor);
  genie.WriteStr(STR_CALDIALOG, buf);

  EEPROM.put(ADDR_CAL, calibration_factor);
  EEPROM.commit();//required on ESP as per https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/examples/eeprom_write/eeprom_write.ino
  scale.set_scale(calibration_factor);
  delay(2000);
  strncpy(remBuf, "Remove all torque", s);
  genie.WriteStr(STR_CALDIALOG, remBuf);
  for (int i = 0; i < 3; i++) {
    strncat(remBuf, ".", s);
    genie.WriteStr(STR_CALDIALOG, remBuf);
    delay(1000);
  }

  scale.tare();
  genie.WriteStr(STR_CALDIALOG, "Done!");
  return calibration_factor;
}
