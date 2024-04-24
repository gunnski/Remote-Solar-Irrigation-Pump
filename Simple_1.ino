#include <Arduino_EdgeControl.h>
// #include "Helpers.h"
// #include "TimeHelpers.h"
#include <DS1307RTC.h>

  volatile bool systemOn = false;
  volatile bool alternateWeek = false;
  volatile int firstRunHour;
  volatile int secondRunHour;
  volatile bool batteryIsGo;
  uint32_t startUp_time;
  uint32_t run_time;
  const int lowerBatteryVoltage = 10;
  const int upperBatteryVoltage = 14.6;

void inverter() {
  if (getAveragePressure() < 20) {
    Serial.println("Pump (inverter) is Off; Switching On...");
    systemOn = true;
    } 
  else if (getAveragePressure() >= 20) {
    Serial.println("Pump (Inverter) is On; Switching Off...");
    systemOn = false;
    }

  Serial.println("Switching Inverter");
  Latching.channelDirection(LATCHING_CMD_8, NEGATIVE);
  delay(500);
  Latching.latch();
  delay(500);
  Latching.channelDirection(LATCHING_CMD_8, POSITIVE);
  delay(500);
  Latching.release();
  delay(500);
  Serial.println("Inverter Switched");
}

void sprinkler(int zone, bool open) {
  if (zone == 1 && open) {
    Serial.println("Switching Latching CMD 1: Zone 1");
    Latching.channelDirection(LATCHING_CMD_1, NEGATIVE), Latching.latch();
  } else if (zone == 1 && !open) {
    Latching.channelDirection(LATCHING_CMD_1, POSITIVE);
    Latching.release();
  }
  if (zone == 2 && open) {
    Serial.println("Switching Latching CMD 2: Zone 2");
    Latching.channelDirection(LATCHING_CMD_2, NEGATIVE), Latching.latch();
  } else if (zone == 2 && !open) {
    Latching.channelDirection(LATCHING_CMD_2, POSITIVE);
    Latching.release();
  }
      if (zone == 3 && open) {
    Serial.println("Switching Latching CMD 3: Zone 3");
    Latching.channelDirection(LATCHING_CMD_3, NEGATIVE), Latching.latch();
  } else if (zone == 3 && !open) {
    Latching.channelDirection(LATCHING_CMD_3, POSITIVE);
    Latching.release();
  }
      if (zone == 4 && open) {
    Serial.println("Switching Latching CMD 4: Zone 4");
    Latching.channelDirection(LATCHING_CMD_4, NEGATIVE), Latching.latch();
  } else if (zone == 4 && !open) {
    Latching.channelDirection(LATCHING_CMD_4, POSITIVE);
    Latching.release();
  }
  if (zone == 5 && open) {
    Serial.println("Switching Latching CMD 5: Zone 5");
    Latching.channelDirection(LATCHING_CMD_5, NEGATIVE), Latching.latch();
  } else if (zone == 5 && !open) {
    Latching.channelDirection(LATCHING_CMD_5, POSITIVE);
    Latching.release();
  }

  if (zone == 6 && open) {
    Serial.println("Switching Latching CMD 6: Zone 6");
    Latching.channelDirection(LATCHING_CMD_6, NEGATIVE), Latching.latch();
  } else if (zone == 6 && !open) {
    Latching.channelDirection(LATCHING_CMD_6, POSITIVE);
    Latching.release();
  }
}

float getBatPercent() {
  float voltage = Power.getVBat();
  float batPercent = ((voltage - lowerBatteryVoltage) / (upperBatteryVoltage - lowerBatteryVoltage) * 100);
  return batPercent;
}

uint16_t getAveragePressure()  {
    constexpr size_t loops { 100 };
    int tot { 0 };
    analogReadResolution(ADC_RESOLUTION);
    Input.enable();
    for (auto i = 0u; i < loops; i++)
        tot += Input.analogRead(INPUT_420mA_CH01);
    Input.disable();
    auto avg = tot / loops;
    return map(avg, 0, 3485, 0, 100) // return mapped values (DO NOT CHANGE ONCE CALIBRATED!)
    // this is the initial best fit for original calibration:
    // --->  y = 27.5x + 731  <---
}

bool it_is_time(uint32_t t, uint32_t t0, uint16_t dt) {
  return ((t >= t0) && (t - t0 >= dt)) ||         // The first disjunct handles the normal case
            ((t < t0) && (t + (~t0) + 1 >= dt));  //   while the second handles the overflow case
}

void setup() {
  Serial.begin(9600), EdgeControl.begin();
  delay(500);
  Input.begin(), Latching.begin(), Relay.begin();
  delay(500);
  Wire.begin(), Expander.begin();
  delay(500);
  Power.on(PWR_3V3);
  Power.on(PWR_VBAT);
  Power.on(PWR_19V);
  delay(500);
  setSystemClock();
  LCD.begin(16, 2), LCD.leftToRight();
  LCD.display(), LCD.backlight();
  LCD.print("Hi!");
  Serial.println("Setup Comlete!");
}

void loop() {
  if (!systemOn) {
    if (batteryIsGo && hour() == firstRunHour) {
      if (weekday() == 2) {
        inverter(), Serial.println("Inverter On"); 
        sprinkler(1, true);
        }
      if (weekday() == 3) {
        inverter(), Serial.println("Inverter On"); 
        sprinkler(2, true);
        }
      if (weekday() == 4) {
        inverter(), Serial.println("Inverter On"); 
        sprinkler(3, true);
        }
    } 
  else if (!systemOn && batteryIsGo && hour() == secondRunHour) {
    if (weekday() == 5) {
      inverter(), Serial.println("Inverter On"); 
      sprinkler(4, true);
    }
    if (weekday() == 5) {
      inverter(), Serial.println("Inverter On"); 
      sprinkler(5, true);
    }
    if (weekday() == 6) {
      inverter(), Serial.println("Inverter On"); 
      sprinkler(6, true);
      // change the runtime hours when the end of the week is reached.
      if (!alternateWeek) {
        alternateWeek = !alternateWeek;
        firstRunHour = 6, secondRunHour = 21;
      } 
      else {
        alternateWeek = !alternateWeek;
        firstRunHour = 21, secondRunHour = 6;
      }
      Serial.print("Setting Alternate week state: "), Serial.println(alternateWeek);
      Serial.print("firstRunHour"), Serial.println(firstRunHour);
      Serial.print("firstRunHour"), Serial.println(secondRunHour);
    }
  }
  }
  uint32_t t = now(); // store the current millis
  if (systemOn && it_is_time(t, startUp_time, 5000)) {
    if (getAveragePressure() <= 10 || getAveragePressure() >= 50 || getBatPercent() <= 20) {
      startUp_time = now();
      inverter();
      sprinkler(1, false), sprinkler(2, false), sprinkler(3, false);
      sprinkler(4, false), sprinkler(5, false), sprinkler(6, false);
    }
  }

  if (systemOn && it_is_time(t, run_time, 15*60*1000)) {
    uint32_t run_time = now();
    inverter();
    sprinkler(1, false), sprinkler(2, false), sprinkler(3, false);
    sprinkler(4, false), sprinkler(5, false), sprinkler(6, false); 
  }

  batteryIsGo = (getBatPercent() > 80) ? true : false;
  Serial.print("Battery Voltage: "), Serial.println(Power.getVBat());
  Serial.print("Battery Percent: "), Serial.print(getBatPercent()), Serial.println("%");
  pressureIsGo = (getAveragePressure() > 15) ? true : false;
  Serial.print("System Pressure: "), Serial.print(getAveragePressure()), Serial.println("PSI");
  
  printLCD();
}

void printLCD() {
  for (auto i = 0; i <= 6; i++) {
    LCD.clear(), LCD.home();
    LCD.print(getAveragePressure());
    LCD.setCursor(0, 1);
    LCD.print(getBatPercent());
    delay(500);
  }
}