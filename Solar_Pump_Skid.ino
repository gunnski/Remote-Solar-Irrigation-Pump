/*
  Wahsington State University
  TSMC Washington (formely WaferTech)

  Operating code for Solar Pump Skid Project

  Last Update: 2024-03-21
  By: Gunnar Blomquist
*/

// Include the following libraries:
#include <Arduino_EdgeControl.h>  // general library for edge

// Finite State Machine: Initialize varaibles:
int state = 0;  // Finite State Machine
bool pressureError = false;
bool lowVoltageError = false;
double pressure = 0;
double voltqge = 0;
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
const int minBatteryPercent = 20;    // the minumim discharge level of battery in percentage
const int maxBatteryPercent = 80;    // the necessary battery percentage threshold needed to operate the system
const int minimumPumpPressure = 10;  // minumin allowed pump pressure in psi
const int PumpDuration = 15;         // Duration of pump operation in minutes

// Advance the queue when called:
void nextZone() { 
  valveZone = (valveZone > numberOfZones) ? valveZone = 1 : ++valveZone;
}

// Define states:
enum State {
  ERRORSTATE,
  IDLE,
  PUMPSTART,
  PUMPRUN,
  PUMPSTOP
};

// this state is called when water tank is empty; wait for reset.
void error(){


};

// This state is called when system is idle.
void idle() {
  // meassure voltage here
  // batteryPercentage = map(voltage, 0,255, 0, 100);
  // battery life in %
  if (batteryPercentage >= maxBatteryPercent) {
    if (!pressureError) {
      // Check time??
    } else {
      state = IDLE;
    }
  } else {
    state = IDLE;
  }
}

// This state is called when the pump should start.
void pumpStart() {
  unsigned long pressureStartTime = millis();
  // turn valve & inverter on

  // get pressure from 4-20ma input here

  if ((pressureStartTime - millis()) > 5000 && pressure < 10) {
    pressureError = true;
    state = PUMPSTOP;
  } else {
    state = PUMPSTART;
  }
}

// This state is called if the pump successfully starts; countdown begins
void pumpRun() {
  // check pressure is >= 10 psi via pressure input
  // check battery life is >= 20% via voltage input
  unsigned long pumpRunMillis = millis();
  if ((pumpRunMillis - millis()) > PumpDuration * 60 * 1000) {
    nextZone();
    state = PUMPSTOP;
  }
  if (pressure < minimumPumpPressure || batteryPercentage < minBatteryPercent) {
    state = PUMPSTOP;
  } else {
    state = PUMPRUN;
  }
}

// This state always follows PUMPRUN state to discontinue the operation.
void pumpStop() {
  // turn valve & inverter off
}

// Setup stuff here to run once:
void setup() {
  // pumpSolenoid = pinMode(1,OUTPUT)
  // valveOne = pinMode(2,OUTPUT);
  // valveTwo = pinMode(3,OUTPUT);
}

// Main code loop here:
void loop() {
  switch (state) {
    case ERRORSTATE:
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
  }
}