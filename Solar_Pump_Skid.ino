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
#include <math.h>

// Finite State Machine: 
  // Define states:
  enum State {
    ERROR,
    IDLE,
    PUMPSTART,
    PUMPRUN,
    PUMPSTOP
  };

  // Initial state:
  int state = IDLE; 

// System constant parameters during operation:
  // Battery and Voltage:
  const float upperBatteryVoltage = 13.6;
  const float lowerBatteryVoltage = 10.5;

        // The pump can operate between these states of charge: 
  const float minStateOfCharge = 20;    // the minumim discharge level of battery in percentage
  const float maxStateOfCharge = 80;    // the necessary battery percentage threshold needed to operate the system
  bool lowVoltageError = false;

  // Plumbing, pressure, and pump:
  const int minimumPumpPressure = 10;  // minumin allowed pump pressure in psi
  
  bool pressureError = false;
  

// Battery ansd volatege related variables: 
float voltage = 0;
float batteryPercentage = 0;
  // Timing the voltage measureing function
  const uint32_t MeasureVoltageInterval = 2000;   // Delay between measurements
  uint32_t voltageTime;                           // Global variable to store the time of the last voltage measurement

// Plumbing, pressure, and pump related variables: 
double pressure = 0;

  // Initialize Relay Outputs here:
  int pumpSolenoid = HIGH;
  int valveOne;
  int valveTwo;
  // etc.

  // Timming the INITIAL pressure measuring function during pump startup:
  const uint32_t initialMeasurePressureInterval = 10*1000;  // Delay between measurements (10 seconds); uses 'pressureTime' to compare

  // Timming the pressure measuring function during pump operaton:
  const uint32_t measurePressureInterval = 4*1000;  // Delay between measurements (5 seconds)
  uint32_t pressureTime;                          // Global variable to store the time of the last voltage measurement

  // Timming the pump runtime limit trigger:
  const int PumpDuration = 15*60*1000;                 // Duration of pump operation in minutes
  uint32_t PumpTime;                       // Global variable to store the time of the last voltage measurement

  // Zone Valve parameters:
  int valveZone = 1;
  const int numberOfZones = 6;

// Helper function to check if we should do something
bool it_is_time(uint32_t t, uint32_t t0, uint16_t dt) {
  return ((t >= t0) && (t - t0 >= dt)) ||         // The first disjunct handles the normal case
            ((t < t0) && (t + (~t0) + 1 >= dt));  //   while the second handles the overflow case
}

// Advance the queue when called:
void nextZone() { 
  valveZone = (valveZone >= numberOfZones) ? valveZone = 1 : ++valveZone;
  Serial.println("Zone: "), Serial.println(valveZone);
};

void measureVoltage() {
  uint32_t vt = millis();                         // Local variable to store the current value of the millis() voltage timer
  if (it_is_time(vt, voltageTime, MeasureVoltageInterval)) {
    voltage = Power.getVBat();
    batteryPercentage = (voltage - lowerBatteryVoltage) / (upperBatteryVoltage - lowerBatteryVoltage) * 100;
    voltageTime = vt;
    // Serial.println(voltage), Serial.println(batteryPercentage);  // de-comment for debug
    if (batteryPercentage < minStateOfCharge) {
      lowVoltageError = true;
    }
    else if (batteryPercentage > maxStateOfCharge) {
      lowVoltageError = false;
    }
  }
};

void measurePressure() {
  // insert an actual average analogRead fn here
  uint32_t pt = millis();                         // Local variable to store the current value of the millis() voltage timer
  if (it_is_time(pt, pressureTime, measurePressureInterval)) {
    nextZone();
    // analogRead()
    pressure = 1;
    pressureTime = pt;
  }
}

// this state is called when water tank is empty; wait for reset.
void error() {
  if (lowVoltageError) {
    LCD.home(), LCD.clear(), LCD.print("ERROR:");
    LCD.setCursor(0,1), LCD.print("Low Voltage!");
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
  LCD.clear(), LCD.home();

  // meassure voltage if voltageTimer interval has elapsed
  measureVoltage();

  // Serial.println("Voltage: ");
  // Serial.println(voltage);
  LCD.clear(), LCD.home(), LCD.print("Battery:");
  LCD.setCursor(0,1), LCD.print(voltage), LCD.setCursor(5,1), LCD.print("V");
  LCD.setCursor(10,1), LCD.print(batteryPercentage),  LCD.setCursor(14,1), LCD.print("%");
  
  delay(2000);

  measurePressure();

  LCD.clear(), LCD.home(), LCD.println("Water Pressure: ");
  LCD.setCursor(0,1), LCD.print(pressure), LCD.setCursor(5,1), LCD.print("PSI");
  // Serial.println("Pressure: ");
  // Serial.println(pressure + "%");

  delay(2000);
  
  LCD.clear(), LCD.home(), LCD.print("Zone: ");
  LCD.setCursor(0,1), LCD.print(valveZone); // LCD.setCursor(3,1), LCD.print("is next");

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
  // Serial.println("PUMP STARTING");
  LCD.clear(), LCD.home(), LCD.print("PUMP STARTING");
  LCD.setCursor(0,1), LCD.print(pressure), LCD.setCursor(5,1), LCD.print("PSI");

  // inverterSolenoid(ON)
  // inverterSwtich(ON)
  // Valve(valveZone, ON)

  measurePressure();

  uint32_t ps = millis();
  if (it_is_time(ps, PumpTime, initialMeasurePressureInterval) && (pressure < minimumPumpPressure)) {
    PumpTime = ps;
    pressureError = true;
    state = PUMPSTOP;
    }
  else if (pressure > minimumPumpPressure) {state = PUMPRUN, PumpTime = millis();}     // If pump succesfully builds pressure, next state and set timer to current millis(); countdown begins
  else {state = PUMPSTART;}    // Keep looping this case until an outcome is met
}


// This state is called if the pump successfully starts
void pumpRun() {
  // Serial.println("PUMP RUNNING");
  LCD.clear(), LCD.home(), LCD.print("PUMP RUNNING");

  measurePressure();
  measureVoltage();
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
  uint32_t pr = millis();
  if (it_is_time(pr, PumpTime, PumpDuration) || pressure < minimumPumpPressure || voltage < minStateOfCharge) {
    nextZone();
    PumpTime = pr;
    state = PUMPSTOP; // stop pump if ANY stop condition is met: past time, low pressure, low voltage
  }

}

// This state always follows PUMPRUN state to discontinue the operation.
void pumpStop() {
  // inverterSolenoid(OFF)
  // inverterSwtich(OFF)
  // Valve(valveZone, OFF)
  // Serial.println("PUMP STOPPING");
  LCD.clear(), LCD.home(); LCD.print("PUMP STOPPING");
  
  state = IDLE;
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

  voltageTime = millis();              // Remember the current value of the millis timer

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