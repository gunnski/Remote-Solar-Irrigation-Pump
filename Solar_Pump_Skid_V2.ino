/*
  Wahsington State University
  TSMC Washington (formely WaferTech)

  Operating code for Solar Pump Skid Project

  Last Update: 2024-03-21
  By: Gunnar Blomquist
*/

// Include the following libraries:
#include <Arduino_EdgeControl.h>  // general library for edge
#include "helpers.h"
#include <Wire.h>
// #include <math.h>

// For the something of the getAveragePressure() funtion
constexpr unsigned int adcResolution { 12 };

// Finite State Machine: 
  // Define states:
  enum State {
    IDLE,
    START,
  };

  // Initial state:
  int state = IDLE; 
//

// System parameters:
  // LCD
  const uint32_t LCD_InfoInterval = 2000;
  const uint32_t LCD_TimeInterval = 10000;
  uint32_t idleTime;
  uint32_t idleTime2;

  // Battery and Voltage:
    float voltage = 0;
    float batteryPercentage = 0;
  //

  // Plumbing, pressure, and pump:
    float pressure = 0;
    const float minStateOfCharge = 20;    // the minumim discharge level of battery in percentage
    const float reqStateOfCharge = 80;    // the necessary battery percentage threshold needed to operate the system
    
    const int minimumPumpPressure = 10;   // minumin allowed pump pressure in psi
    bool lowPressure = true;
    
    // Timming the INITIAL pressure measuring function during pump startup:
    const uint32_t initialMeasurePressureInterval = 10*1000;  // Delay between measurements (10 seconds); uses 'pumpStartTime' to compare

    // Timming the pressure measuring function during pump operaton:
    const uint32_t measurePressureInterval = 4*1000;  // Delay between measurements (5 seconds)
    uint32_t pumpStartTime;                          // Global variable to store the current time of the pump start

    // Timming the pump runtime limit trigger:
    const int PumpDuration = 15*60*1000;                 // Duration of pump operation in minutes
    uint32_t PumpTime;                       // Global variable to store the current time of the pump
  //

  // Initialize Relay Outputs here:
    int pumpSolenoid = HIGH;
    int valveOne;
    int valveTwo;
    // etc.

    // Zone Valve parameters:
    int zoneValve = 1;
    const int numberOfZones = 6;
//

// Measure voltage function
float getBatPercent() {
  const float upperBatteryVoltage = 13.6;
  const float lowerBatteryVoltage = 10.5;
  voltage = Power.getVBat();
  return batteryPercentage = (voltage - lowerBatteryVoltage) / (upperBatteryVoltage - lowerBatteryVoltage) * 100;
}

//
float getAveragePressure()  {
  Power.on(PWR_19V);
  constexpr size_t loops { 100 };
  constexpr float toV { 3.3f / float { (1 << adcResolution) - 1 } };
  constexpr float rDiv { 17.4f / ( 10.0f + 17.4f) };

  int tot { 0 };

  for (auto i = 0u; i < loops; i++)
      tot += Input.analogRead(0);
  return pressure = static_cast<float>(tot) * toV / static_cast<float>(loops);

  Power.off(PWR_19V);
}

// This state is called when system is waiting for the start time             | ALMOST DONE.
void idle() {
  uint32_t now = millis();

  Serial.println("Idling...");

  // Update LCD every few seconds
  if (it_is_time(now, idleTime, LCD_InfoInterval)) {

    Serial.println("Updating LCD...");
    idleTime = now;
    LCD.clear(), LCD.home();
    LCD.print("Zone: "), LCD.setCursor(6,0), LCD.print(zoneValve);
    LCD.setCursor(0,1), LCD.print("Batt:"), LCD.setCursor(5,1), LCD.print(batteryPercentage), LCD.setCursor(9,1), LCD.print("%");
    delay(2000);
  }
  String dateTime = getLocaltime();
  String date = dateTime.substring(0, 10);
  String Time = dateTime.substring(11, 20);

  LCD.home(), LCD.clear();
  LCD.print("Date:"), LCD.setCursor(6,0), LCD.print(date);
  LCD.setCursor(0,1), LCD.print("Time:"), LCD.setCursor(7,1), LCD.print(Time);
}

// This state is called to start the process
void start() {
  pressure = getAveragePressure();
  batteryPercentage = getBatPercent();

  uint32_t pr = millis();

  if (it_is_time(pr, PumpTime, PumpDuration) || pressure < minimumPumpPressure || voltage < minStateOfCharge) {
    PumpTime = pr;
    zoneValve = (zoneValve >= numberOfZones) ? zoneValve = 1 : ++zoneValve;    // advance the queue
    state = IDLE; // stop pump if ANY stop condition is met: past time, low pressure, low voltage
  }

  // Serial.println("PUMP RUNNING");
  // Serial.println("Pressure: ");
  // Serial.println(pressure + "%");
  LCD.clear(), LCD.home(), LCD.print("PUMP RUNNING");
  LCD.println("Pressure:"), LCD.setCursor(10,1), LCD.print(pressure), LCD.setCursor(14,1), LCD.print("PSI");
}

// Helper function to check if we should do something                         | DONE.
bool it_is_time(uint32_t t, uint32_t t0, uint16_t dt) {
  return ((t >= t0) && (t - t0 >= dt)) ||         // The first disjunct handles the normal case
            ((t < t0) && (t + (~t0) + 1 >= dt));  //   while the second handles the overflow case
}

// Setup stuff here to run once:                                              | NEEDS RELAYS INITIALIZED.
void setup() {
  pinMode(POWER_ON, INPUT);
  Serial.begin(9600);
  for (auto timeout = millis() + 2500l; !Serial && millis() < timeout; delay(250))
    ;
  Serial.println("WaferTech | WSUV  Solar Irrigation Pump Project");
  EdgeControl.begin();

  Power.on(PWR_3V3);
  Power.on(PWR_VBAT);

  Wire.begin();

  // LCD BEGIN
    LCD.begin(16, 2);
    LCD.backlight();
    LCD.clear(), LCD.home();

  RealTimeClock.begin();
  // Enables Alarm on the RTC

  RealTimeClock.enableAlarm();
  // Set the minutes at which the alarm should rise
  // Trigger in a minute
  auto minutes = RealTimeClock.getMinutes();
  RealTimeClock.setMinuteAlarm(minutes + 1);

  attachInterrupt(
    digitalPinToInterrupt(IRQ_RTC), [] { alarmFlag = true; }, FALLING);
  
  Serial.println("Setting up...");
  auto startNow = millis() + 2500;
  while (!Serial && millis() < startNow)
      ;

  Serial.println("Testing LCD for Arduino Edge Control");

  delay(1000);

  Expander.pinMode(EXP_FAULT_SOLAR_PANEL, INPUT);
  Expander.pinMode(EXP_FAULT_5V, INPUT);

  // pumpSolenoid = pinMode(1,OUTPUT)
  // valveOne = pinMode(2,OUTPUT);
  // valveTwo = pinMode(3,OUTPUT);

  if (digitalRead(POWER_ON) == LOW) {
    Serial.println("Resetting the RTC to Sketch Build Datetime!");
    auto buildDateTime = getBuildDateTime();
    RealTimeClock.setEpoch(buildDateTime);
    Serial.print("Build ");

  state = IDLE;
  Serial.println("Setup Complete");
  }
}

// Main code loop here:                                                       | DONE.
void loop() {
  uint32_t now = millis();
  batteryPercentage = getBatPercent();
  pressure = getAveragePressure();

  switch (state) {
    case START:
      delay(1000);
      LCD.clear(), LCD.home();
      start();
      LCD.clear(), LCD.home();
      break;
    case IDLE:
      delay(1000);
      idle();
      break;
  }
}

bool getStatus(float batPercent = 0, float pumpPress = 0) {
  now = millis();

    // Check if pump should run: 
  //    1) if battery percentage is above 80%-ish, 
  if (batPercent >= reqStateOfCharge) {
    Serial.println("Battery: GO!");
    // then 2) if 24 has passed, 
    if (RealTimeClock.getHours()) {
      Serial.println("Time: GO!");
      // and finally 3) if no pressure error is active.
      if (!lowPressure) {
          Serial.println("Pressure Status: GO!"), Serial.println("Proceeding...");
          LCD.setCursor(13,0), LCD.print("~~~");
          PumpTime = now;
          return true;
      } else {
          Serial.println("Pressure Status: NO GO!");
          LCD.setCursor(13,0), LCD.print("~~!");
          return false;
          }
    } else {
      Serial.println("Time: NO GO!");
      state = IDLE;
      LCD.setCursor(13,0), LCD.print("~!~");
      return false;
      }
  } else {
    Serial.println("Battery NO GO!");
    state = IDLE;
    LCD.setCursor(13,0), LCD.print("!~~");
    return false;
    }
  
}

void updateLCD(String status = "Idle", float data1 = 0, float data2 = 0) {
  LCD.backlight(), LCD.clear(), LCD.home();

  LCD.setCursor(12,0), LCD.print(status);

  LCD.print("Date:"), LCD.setCursor(6,0), LCD.print(date);
  LCD.setCursor(0,1), LCD.print("Time:"), LCD.setCursor(7,1), LCD.print(Time);

  LCD.clear(), LCD.home();
  LCD.print("Zone: "), LCD.setCursor(6,0), LCD.print(zoneValve);
  LCD.setCursor(0,1), LCD.print("Batt:"), LCD.setCursor(5,1), LCD.print(batteryPercentage), LCD.setCursor(9,1), LCD.print("%");
}

