/*
  Wahsington State University
  TSMC Washington (formely WaferTech)

  Operating code for Solar Pump Skid Project

  Last Update: 2024-03-21
  By: Gunnar Blomquist
*/

#include <Arduino_EdgeControl.h>  // general library for edge
// #include "Helpers.h"             // library for RTC
#include <list>  // for zone Ordering control


int state = 0;  // Finite State Machine
bool pressureError = false;
bool lowVoltageError = false;
double pressure = 0;
double voltqge = 0;
double batteryPercentage = 0;

std::list<int> zoneOrder = { 1, 2, 3, 4, 5, 6 };
int valveZone = 1;
int numberOfZones = 6;

const int minBatteryPercent = 20;    // the minumim discharge level of battery in percentage
const int maxBatteryPercent = 80;    // the necessary battery percentage threshold needed to operate the system
const int minimumPumpPressure = 10;  // minumin allowed pump pressure in psi
const int PumpDuration = 15;         // Duration of pump operation in minutes

void nextZone() {
  /* Advance the queue when called */
  valveZone = (valveZone > numberOfZones) ? valveZone = 1 : ++valveZone;
}

enum State {
  ERRORSTATE,
  IDLE,
  PUMPSTART,
  PUMPRUN,
  PUMPSTOP
};

void error() {
  // call this state when water tank is empty and wait for reset

};

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

void pumpStop() {
  // turn valve & inverter off
}

int pumpSolenoid = HIGH;
int valveOne;
int valveTwo;

void setup() {
  // pumpSolenoid = pinMode(1,OUTPUT)
  // valveOne = pinMode(2,OUTPUT);
  // valveTwo = pinMode(3,OUTPUT);
}

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
