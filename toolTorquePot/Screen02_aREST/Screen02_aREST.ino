#include <genieArduino.h>
#include "HX711.h"
#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <aREST.h>
#include <aREST_UI.h>

Genie genie;
#define RESETLINE 5  // Change this if you are not using an Arduino Adaptor Shield Version 2 (see code below)

#define DOUT  D5
#define CLK  D6

#define BUZZER 4

HX711 scale;

// Create aREST instance
aREST_UI rest = aREST_UI();

// WiFi parameters
const char* ssid = "NIKKEN-WAP";
const char* password = "2080774004";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);



float calibration_factor;
float testing = 0.00;
int measuredTorque = 0;
int previousTorque = 0;
int scaledTorque = 0;
float scaleFactor = 10.00;
float gaugeMax = 100.00;

String measuredTstring;
String TorqueSetpointString;

int model = 0;
int Csize = 0;

bool holdOn = 0;


bool button = 0;

String models[] = {"Multi-Lock ", "Slim-Chuck/Major Dream", "Victory-Chuck "};

String Chuck[3][12] = {{"C12", "C16", "C20", "C22", "C25", "C32", "C42", "C50"},
  {"SK6 ~dia 3mm", "SK6 dia 4mm~6mm", "SK10 ~dia 3mm", "SK10 dia 4mm~6mm", "SK10 dia 8mm~10mm", "SK16 dia 4mm~6mm", "SK16 dia 8mm~10mm", "SK16 dia 12mm~", "SK20 dia 8mm~10mm", "SK20 dia 12mm~", "SK25 dia 8mm~10mm", "SK25 dia 12mm~"},
  {"VCK6 ~dia 3mm", "VCK6 dia 3mm~6mm", "VCK13 ~dia 5mm", "VCK13 dia 5mm~12mm"}
};

int quantities[] = {8, 12, 4};

int noModels = 3;

int TorqueSetpoint[3][12] = {
  {24, 28, 32, 36, 42, 80, 95, 100},
  {20, 30, 25, 40, 45, 50, 65, 75, 70, 90, 85, 100},
  {30, 50, 60, 75}
};

void setup()
{

  Serial.begin(256000);
  Serial.swap();

  


  genie.Begin(Serial);   // Use Serial0 for talking to the Genie Library, and to the 4D Systems display
  genie.AttachEventHandler(myGenieEventHandler); // Attach the user function Event Handler for processing events

  pinMode(BUZZER, OUTPUT); //Buzzer
  digitalWrite(BUZZER, HIGH);
  delay(50);
  digitalWrite(BUZZER, LOW);

  pinMode(RESETLINE, OUTPUT);  // Set D4 on Arduino to Output
  digitalWrite(RESETLINE, LOW);  // Reset the Display via D4
  delay(100);
  digitalWrite(RESETLINE, HIGH);  // unReset the Display via D4

  delay (3500); //let the display start up after the reset (This is important)

  genie.WriteObject(GENIE_OBJ_FORM, 1, 0);

  Serial1.begin(115200);  // Serial0 @ 200000 (200K) Baud

  setupaREST();

  genie.WriteStr(0, models[model]);
  genie.WriteStr(1, Chuck[model][Csize]);
  TorqueSetpointString = String(TorqueSetpoint[model][Csize]) + " Nm";
  genie.WriteStr(2, TorqueSetpointString);

  //pinMode(13, OUTPUT); //LED
  //pinMode(23, OUTPUT); //Power for load cell
  //digitalWrite(23, HIGH); //Turn on load cell
  digitalWrite(BUZZER, HIGH);
  delay(50);
  digitalWrite(BUZZER, LOW);
  Serial1.println("Copyright NIKKEN Kosakusho Europe Ltd. 2019");
  Serial1.println("Welcome!");

  EEPROM.get(0, calibration_factor);
  calibration_factor = 6387;
  Serial1.print("Calibration Factor= ");
  Serial1.println(calibration_factor);
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial1.println("To calibrate, enter C:");
  scaleFactor = gaugeMax / TorqueSetpoint[model][Csize];

  //calibrateScale();

}

void loop()
{
  /*
    if (Serial1.available()>0){
    if (Serial1.read() == 'C'){
      calibrateScale();
    }
    }*/

 WiFiClient client = server.available();

  if (client.available()) {
    rest.handle(client);
  }
  

  //genie.DoEvents();// This calls the library each loop to process the queued responses from the display

  measuredTorque = scale.get_units();
  //Scale Torque to bar 0-100
  scaledTorque = scaleFactor * measuredTorque;

  if (scaledTorque >= 100) {
    scaledTorque = 100;
    digitalWrite(D5, HIGH);
  }
  else {
    digitalWrite(D5, LOW);
  }
  if (scaledTorque < 0) {
    scaledTorque = 0;
  }


  if (holdOn == 1) {
    if (measuredTorque > previousTorque) {
      //genie.WriteObject(GENIE_OBJ_GAUGE, 0, scaledTorque);
      measuredTstring = String(measuredTorque) + " Nm";
      //genie.WriteStr(3, measuredTstring);
      previousTorque = measuredTorque;
    }
  }

  if (holdOn == 0) {
    //genie.WriteObject(GENIE_OBJ_GAUGE, 0, scaledTorque);
    measuredTstring = String(measuredTorque) + " Nm";
    //genie.WriteStr(3, measuredTstring);
    previousTorque = 0;
  }

}


void myGenieEventHandler(void)
{
  genieFrame Event;
  genie.DequeueEvent(&Event); // Remove the next queued event from the buffer, and process it below

  //If the cmd received is from a Reported Event (Events triggered from the Events tab of Workshop4 objects)
  if (Event.reportObject.cmd == GENIE_REPORT_EVENT)
  {
    if (Event.reportObject.object == GENIE_OBJ_4DBUTTON)
    {
      if (Event.reportObject.index == 0)
      {
        if (model < noModels - 1) {
          model = model + 1;
          Csize = 0;
        }
        else {
          model = 0;
          Csize = 0;
        }
        genie.WriteStr(0, models[model]);
        genie.WriteStr(1, Chuck[model][Csize]);
        TorqueSetpointString = String(TorqueSetpoint[model][Csize]) + " Nm";
        genie.WriteStr(2, TorqueSetpointString);
        scaleFactor = gaugeMax / TorqueSetpoint[model][Csize];
      }
      if (Event.reportObject.index == 1)
      {
        if (Csize < quantities[model] - 1) {
          Csize = Csize + 1;
        }
        else {
          Csize = 0;
        }
        genie.WriteStr(0, models[model]);
        genie.WriteStr(1, Chuck[model][Csize]);
        TorqueSetpointString = String(TorqueSetpoint[model][Csize]) + " Nm";
        genie.WriteStr(2, TorqueSetpointString);
        scaleFactor = gaugeMax / TorqueSetpoint[model][Csize];
      }

      if (Event.reportObject.index == 3) {
        holdOn = genie.GetEventData(&Event);
      }
      if (Event.reportObject.index == 2) {
        scale.tare();
      }
    }
  }
}

void calibrateScale(void) {
  Serial1.print("Remove all torque");
  for (int i = 0; i < 3; i++) {
    Serial1.print(".");
    delay(1000);
  }
  Serial1.println(".");
  scale.set_scale(1);//Sets scale back to 1
  scale.tare(3); //Takes 3 tare vals
  Serial1.print("Apply 20nm of torque");
  for (int i = 0; i < 3; i++) {
    Serial1.print(".");
    delay(1000);
  }
  Serial1.println(".");
  measuredTorque = scale.get_value(5); //This is tared value divided by 1, as scale set back to 1
  Serial1.print("Measured input: ");
  Serial1.println(measuredTorque);

  Serial1.print("Calibration factor was: ");
  Serial1.println(calibration_factor);
  calibration_factor = measuredTorque / 20;
  Serial1.print("Calibration factor is now: ");
  Serial1.println(calibration_factor);

  Serial1.print("Remove all torque");
  for (int i = 0 ; i < 3 ; i++) {
    Serial1.print(".");
    delay(1000);
  }
  Serial1.println(".");

  EEPROM.put(0, calibration_factor);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial1.println("Done!");

}

int calibrateScaleFromREST(String cmd){
  Serial1.println(cmd);
  calibrateScale();
  return 1;
}

void setupaREST(){
   // Set the title
  rest.title("aREST UI Demo");

  // Create button to control pin 5
  rest.button(4);

  // Init variables and expose them to REST API

  rest.variable("torque", &measuredTorque);
  rest.variable("ca_factor", &calibration_factor);

  // Labels
  rest.label("torque");
  rest.label("ca_factor");

  // Function to be exposed
  rest.function("cal", calibrateScaleFromREST);

  // Give name and ID to device
  rest.set_id("112525");
  rest.set_name("T-MAX");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial1.print(".");
  }
  Serial1.println("");
  Serial1.println("WiFi connected");

  // Start the server
  server.begin();
  Serial1.println("Server started");

  // Print the IP address
  Serial1.println(WiFi.localIP());
}
