/*
  Wahsington State University
  TSMC Washington (formely WaferTech)

  Operating code for Solar Pump Skid Project

  Last Update: 2024-03-21
  By: Gunnar Blomquist
*/

#include <Arduino_EdgeControl.h>    // general library for edge
// #include "Helpers.h"             // library for RTC

int state = 0;
bool pressureError = false;
bool lowVoltageError = false;
int pressure = 0;
int batteryPercent = 100;

void idle() {
  if (!pressureError) {

    // meassure voltage here

    // battery life in %
    if (batteryPercent >= 20) {

      // Check time is between 4:00-6:00 AM

    }
    else {
      state = 1; // Idle
    }
  }
}

void pumpStart() {
  unsigned long pressureStartTime = millis();
  // turn valve & inverter on

  // get pressure from 4-20ma input here

  if ((pressureStartTime - millis()) > 5000 and pressure < 10) {
    pressureError = true;
    state = 4; // pumpStop

    else {
      state = 3; // pumpRun
    }
  }

  void pumpRun() {
    // check pressure is >= 10 psi via pressure input
    // check battery life is >= 20% via voltage input
  }

  void pumpStop() {
    // turn valve & inverter off

  }

  void setup() {

  }

  void loop() {
    switch (state) {
      case (1): {
          idle();
          break;
        }
      case (2): {
          pumpStart();
          break;
        }
      case (3): {
          pumpRun();
          break;
        }
      case (4): {
          pumpStop();
          break;
        }
    }
  }
