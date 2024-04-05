/*
  Wahsington State University
  TSMC Washington (formely WaferTech)

  Operating code for Solar Pump Skid Project

  Last Update: 2024-03-21
  By: Gunnar Blomquist
*/

// Include the following libraries:
#include <Arduino_EdgeControl.h>  // general library for edge
#include <Wire.h>

// Define states:
enum State {
  ERROR,
  IDLE,
  PUMPSTART,
  PUMPRUN,
  PUMPSTOP
};

// Finite State Machine: Initialize varaibles:
int state = IDLE;  // Finite State Machine

bool pressureError = false;
bool lowVoltageError = false;

double pressure = 0;
double voltage = 0;
double batteryPercentage = 0;

// Initialize Relay Outputs here:
int pumpSolenoid = HIGH;
int valveOne;
int valveTwo;
  // etc.

// Zone Valve parameters:
int valveZone = 1;
const int numberOfZones = 6;

// System constant parameters during operation:
const int upperBatteryVoltage = 13.6;
const int lowerBatteryVoltage = 10.5;
const int minBatteryPercent = 20;    // the minumim discharge level of battery in percentage
const int maxBatteryPercent = 80;    // the necessary battery percentage threshold needed to operate the system
const int minimumPumpPressure = 10;  // minumin allowed pump pressure in psi
const int PumpDuration = 15;         // Duration of pump operation in minutes

// Advance the queue when called:
void nextZone() { 
  valveZone = (valveZone > numberOfZones) ? valveZone = 1 : ++valveZone;
};

auto measureVoltage() {
  double vbat = Power.getVBat();
  auto batPercent = map(vbat,lowerBatteryVoltage,upperBatteryVoltage,0,100);
  // Serial.println("Battery Percentage: ");
  Serial.println(batPercent, vbat);
  return batPercent, vbat;
};

double measurePressure() {
  // insert an actual average analogRead fn here
  double pressure = 1;
  return pressure;
}

// this state is called when water tank is empty; wait for reset.
void error() {
  if (lowVoltageError) {
    LCD.home();
    LCD.clear();
    LCD.print("ERROR:");
    LCD.setCursor(0,1);
    LCD.print("Low Voltage!");
    // Serial.println("ERROR");
    delay(2000);
    state = IDLE;
  }

  if (pressureError) {
    LCD.home();
    LCD.clear();
    LCD.setCursor(0,0);
    LCD.print("Check tank");
    LCD.setCursor(0,1);
    LCD.print("water level!");
    // Serial.println("Check tank water level!");
    delay(2000);
    state = ERROR;
  }
};

// This state is called when system is idle.
void idle() {
  LCD.clear();
  LCD.home();

  Serial.println("IDLE");
  LCD.print("IDLE");

  delay(2000);

  // meassure voltage here
  batteryPercentage, voltage = measureVoltage();

  LCD.clear();
  LCD.home();
  LCD.println("Battery Voltage:");
  LCD.setCursor(0,1);
  LCD.print(voltage);
  // Serial.println("Voltage: ");
  // Serial.println(voltage);

  delay(2000);

  LCD.clear();
  LCD.home();
  LCD.println("Battery Percent:");
  LCD.setCursor(0,1);
  LCD.print(batteryPercentage);
  Serial.println("Battery Percentage: ");
  Serial.println(batteryPercentage);

  delay(2000);

  pressure = measurePressure();

  LCD.clear();
  LCD.home();
  LCD.println("Water Pressure: ");
  LCD.setCursor(0,1);
  LCD.print(pressure);
  // Serial.println("Pressure: ");
  // Serial.println(pressure + "%");

  delay(2000);
  // batteryPercentage = map(voltage, 0,255, 0, 100);
  // battery life in %
  // if (batteryPercentage >= maxBatteryPercent) {
  //   if (!pressureError) {
  //     // Check time??
  //   } else {
  //     state = IDLE;
  //   }
  // } else {
  //   state = IDLE;
  // }
  state = IDLE;
}

// This state is called when the pump should start.
void pumpStart() {
  Serial.println("03 PUMP STARTING");
  LCD.clear();
  LCD.backlight();
  LCD.home(); // go home
  LCD.print("03 PUMP STARTING");
  delay(2000);
  LCD.noBacklight();
  state = PUMPRUN;
  unsigned long pressureStartTime = millis();
  // turn valve & inverter on

  // get pressure from 4-20ma input here

  // if ((pressureStartTime - millis()) > 5000 && pressure < 10) {
  //   pressureError = true;
  //   state = PUMPSTOP;
  // } else {
  //   state = PUMPSTART;
  // }
}

// This state is called if the pump successfully starts; countdown begins
void pumpRun() {
  Serial.println("04  PUMP RUNNING");
  LCD.clear();
  LCD.backlight();
  LCD.home(); // go home
  LCD.print("04  PUMP RUNNING");
  delay(2000);
  LCD.noBacklight();
  state = PUMPSTOP;
  // check pressure is >= 10 psi via pressure input
  // check battery life is >= 20% via voltage input
  // unsigned long pumpRunMillis = millis();
  // if ((pumpRunMillis - millis()) > PumpDuration * 60 * 1000) {
  //   nextZone();
  //   state = PUMPSTOP;
  // }
  // if (pressure < minimumPumpPressure || batteryPercentage < minBatteryPercent) {
  //   state = PUMPSTOP;
  // } else {
  //   state = PUMPRUN;
  // }
}

// This state always follows PUMPRUN state to discontinue the operation.
void pumpStop() {
  LCD.clear();
  LCD.backlight();
  Serial.println("05 PUMP STOPPING");
  LCD.home(); // go home
  LCD.print("05 PUMP STOPPING");
  delay(2000);
  LCD.noBacklight();
  state = ERROR;
  // turn valve & inverter off
}


// Setup stuff here to run once:
void setup() 
{
  EdgeControl.begin();

  Serial.begin(9600);
  Serial.println("Setting up...");
  auto startNow = millis() + 2500;
  while (!Serial && millis() < startNow)
      ;

  delay(1000);
  Serial.println("Testing LCD for Arduino Edge Control");

  Power.on(PWR_3V3);
  Power.on(PWR_VBAT);

  Wire.begin();
  delay(500);

  LCD.begin(16, 2);
  LCD.backlight();

  Expander.pinMode(EXP_FAULT_SOLAR_PANEL, INPUT);
  Expander.pinMode(EXP_FAULT_5V, INPUT);

  // pumpSolenoid = pinMode(1,OUTPUT)
  // valveOne = pinMode(2,OUTPUT);
  // valveTwo = pinMode(3,OUTPUT);
  Serial.println("Setup Complete");

}
// Main code loop here:
void loop() {

  switch (state) {
    case ERROR:
      error();
      break;
    case IDLE:
      idle();
      break;
    case PUMPSTART:
      pumpStart();
      break;
    case PUMPRUN:
      pumpRun();
      break;
    case PUMPSTOP:
      pumpStop();
      break;
    default:
      state = IDLE;
  }
}