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
// #include <math.h>

// For the something of the getAveragePressure() funtion
constexpr unsigned int adcResolution { 12 };

// Finite State Machine: 
  // Define states:
  enum State {
    ERROR,
    IDLE,
    START,
    RUN,
    STOP
  };

  // Initial state:
  int state = IDLE; 

// System parameters:
  // LCD
  const uint32_t LCD_Interval = 2000;
  uint32_t idleTime;

  // Battery and Voltage:
    const float upperBatteryVoltage = 13.6;
    const float lowerBatteryVoltage = 10.5;

    float voltage = 0;
    float batteryPercentage = 0;

  // Plumbing, pressure, and pump:
    float pressure = 0;
    const float minStateOfCharge = 20;    // the minumim discharge level of battery in percentage
    const float reqStateOfCharge = 80;    // the necessary battery percentage threshold needed to operate the system
    bool lowVoltageError = false;         // bool
    const int minimumPumpPressure = 10;   // minumin allowed pump pressure in psi


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
    int zoneValve = 1;
    const int numberOfZones = 6;
//

// Helper function to check if we should do something                         | DONE.
bool it_is_time(uint32_t t, uint32_t t0, uint16_t dt) {
  return ((t >= t0) && (t - t0 >= dt)) ||         // The first disjunct handles the normal case
            ((t < t0) && (t + (~t0) + 1 >= dt));  //   while the second handles the overflow case
}

// Advance the queue when called.                                             | DONE. --> GOT NIXED, SEE PUMP RUN...
  // int nextZone() { 
  //   return zoneValve = (zoneValve >= numberOfZones) ? zoneValve = 1 : ++zoneValve;
  //   // Alternatively:
  //   // zoneValve - ++zoneValve % numberOfZones;     <<-----  Uncomment & use
  //   // Serial.println("Zone: "), Serial.println(zoneValve);
  // };
//

// Measure voltage function
void getBatPercent() {
  batteryPercentage = (Power.getVBat() - lowerBatteryVoltage) / (upperBatteryVoltage - lowerBatteryVoltage) * 100;
}

// bool lowVoltage() {
//   return (voltage < maxStateOfCharge);

// }
void getAveragePressure()  {
  Power.on(PWR_19V);
  constexpr size_t loops { 100 };
  constexpr float toV { 3.3f / float { (1 << adcResolution) - 1 } };
  constexpr float rDiv { 17.4f / ( 10.0f + 17.4f) };

  int tot { 0 };

  for (auto i = 0u; i < loops; i++)
      tot += Input.analogRead(0);
  pressure = static_cast<float>(tot) * toV / static_cast<float>(loops);

  Power.off(PWR_19V);
}

bool lowPressure() {
  return (pressure < minimumPumpPressure);
}

// this state is called when water tank is empty; wait for reset.             | DONE - COULD BE BETTER
void error() {
  Power.off(PWR_3V3);
  Power.off(PWR_VBAT);

  if (lowPressure()) {
    // Serial.println("Check tank water level!");
    LCD.home(), LCD.clear(), LCD.print("Check tank");
    LCD.setCursor(0,1), LCD.print("water level!");
    delay(2000);

    LCD.home(), LCD.clear(), LCD.print("REQUIRES SYSTEM");
    LCD.setCursor(5,1), LCD.print("RESET");
    delay(2000);
    state = ERROR;
  }
  else if (lowVoltageError) {
    LCD.home(), LCD.clear(), LCD.print("ERROR:");
    LCD.setCursor(0,1), LCD.print("Low Voltage!");
    // Serial.println("ERROR");
    delay(2000);
    state = IDLE;
  }
  else {state = IDLE;}
};

// This state is called when system is waiting for the start time             | ALMOST DONE.
void idle() {
  (batteryPercentage = getBatPercent() < reqStateOfCharge) ? state = ERROR: state = IDLE;

  // LCD.clear(), LCD.home();
  LCD.print("Zone:"), LCD.setCursor(6,0), LCD.print(zoneValve);
  LCD.setCursor(0,1), LCD.print("Batt:"), LCD.setCursor(5,1), LCD.print(voltage), LCD.setCursor(10,1), LCD.print("V");
  LCD.setCursor(11,1), LCD.print(batteryPercentage), LCD.setCursor(15,1), LCD.print("%");
}

// This state is called when the pump should start.                           | NEEDS SWITCHES IMPLEMENTED.
void start() {
  // Serial.println("PUMP STARTING");
  LCD.clear(), LCD.home(), LCD.print("PUMP STARTING");
  LCD.setCursor(0,1), LCD.print(pressure), LCD.setCursor(5,1), LCD.print("PSI");

  // inverterSolenoid(ON)
  // inverterSwtich(ON)
  // Valve(zoneValve, ON)
  getAveragePressure();
  

  uint32_t ps = millis();
  if (it_is_time(ps, PumpTime, initialMeasurePressureInterval) && lowPressure()) {
    PumpTime = ps;
    state = STOP;
    }
  else if (pressure > minimumPumpPressure) {
    state = RUN, PumpTime = millis();     // If pump succesfully builds pressure, next state and set timer to current millis(); countdown begins
    }     
  else {state = START;}    // Keep looping this case until an outcome is met
}

// This state is called if the pump successfully starts.                      | DONE.
void run() {
  getAveragePressure();
  getBatPercent();

  uint32_t pr = millis();
  if (it_is_time(pr, PumpTime, PumpDuration) || pressure < minimumPumpPressure || voltage < minStateOfCharge) {
    zoneValve - ++zoneValve % numberOfZones;
    PumpTime = pr;
    state = STOP; // stop pump if ANY stop condition is met: past time, low pressure, low voltage
  }

  // Serial.println("PUMP RUNNING");
  // Serial.println("Pressure: ");
  // Serial.println(pressure + "%");
  LCD.clear(), LCD.home(), LCD.print("PUMP RUNNING");
  LCD.println("Pressure:"), LCD.setCursor(10,1), LCD.print(pressure), LCD.setCursor(14,1), LCD.print("PSI");
}

// This state always follows RUN state to discontinue the operation.      | NEEDS SWITCHES IMPLEMENTED.
void stop() {
  // inverterSolenoid(OFF)
  // inverterSwtich(OFF)
  // Valve(zoneValve, OFF)
  // Serial.println("PUMP STOPPING");
  LCD.clear(), LCD.home(); LCD.print("PUMP STOPPING");
  
  state = IDLE;
}

// Setup stuff here to run once:                                              | NEEDS SWITCHES INITIALIZED.
void setup() {
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
  LCD.clear(), LCD.home();

  Expander.pinMode(EXP_FAULT_SOLAR_PANEL, INPUT);
  Expander.pinMode(EXP_FAULT_5V, INPUT);

  // pumpSolenoid = pinMode(1,OUTPUT)
  // valveOne = pinMode(2,OUTPUT);
  // valveTwo = pinMode(3,OUTPUT);


  Serial.println("Setup Complete");
}

// Main code loop here:                                                       | DONE.
void loop() {
  switch (state) {
    case ERROR:
      error();
      break;
    case IDLE:
      idle();
      break;
    case START:
      start();
      break;
    case RUN:
      run();
      break;
    case STOP:
      stop();
      break;
    default:
      state = IDLE;
  }
}

