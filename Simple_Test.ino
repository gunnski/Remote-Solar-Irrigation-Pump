#include <Arduino_EdgeControl.h>

void setup() {
    Serial.begin(9600);
    while (!Serial)
        ;
    EdgeControl.begin(), Wire.begin(), Input.begin(), Latching.begin(), Relay.begin();
    delay(500);
    Power.on(PWR_3V3);
    Power.on(PWR_VBAT);
    Power.on(PWR_19V);
    delay(500);
    Expander.begin();

    LCD.begin(16, 2);
    LCD.leftToRight();
    LCD.display(), LCD.backlight();
    LCD.print("Hi!");

    Serial.println("Setup Comlete!");
}

void loop() {
  //  Test Zone 1
    inverter();
    Serial.println("Inverter On"); 
    sprinkler(1, true);
    printLCD();
    sprinkler(1, false);
    inverter();
    Serial.println("Inverter Off");

   delay(5000);

  // Test Zone 2
    inverter();
    Serial.println("Inverter On");
    sprinkler(2, true);
    printLCD();
    sprinkler(2, false);
    Serial.println("Inverter Off");
    inverter();

    delay(5000);

  // Test Zone 3
    inverter();
    Serial.println("Inverter On");
    sprinkler(3, true);
    printLCD();
    sprinkler(3, false);
    Serial.println("Inverter Off");
    inverter();

    delay(5000);

  // Test Zone 4
    inverter();
    Serial.println("Inverter On");
    sprinkler(4, true);
    printLCD();
    sprinkler(4, false);
    Serial.println("Inverter Off");
    inverter();

    delay(5000);

  //  Test Zone 5
    inverter();
    Serial.println("Inverter On");
    sprinkler(5, true);
    printLCD();
    sprinkler(5, false);
    Serial.println("Inverter Off");
    inverter();

    delay(5000);

  // Test Zone 6
    inverter();
    Serial.println("Inverter On");
    sprinkler(6, true);
    printLCD();
    sprinkler(6, false);
    Serial.println("Inverter Off");
    inverter();

    delay(5000);
}

float getBatPercent() {
  float voltage = Power.getVBat();
  float batPercent = abs(((voltage - 20) / (13.6 - 11.5) * 100));
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
    return tot / loops;
}

// void sprinklerRun(bool status) {
  //   if (status) {
  //     Serial.print("Starting up Sprinkler system...");
  //     delay(250);
  //     Serial.println("Switching Latching CMD 3: Inverter");
  //   // Test MR 3: Start Inverter
  //     Latching.channelDirection(LATCHING_CMD_3, POSITIVE), Latching.latch();
  //     delay(250);
  //     Latching.release(); Latching.channelDirection(LATCHING_CMD_3, NEGATIVE);
  //     delay(250);
  //     Latching.latch();
  //   Serial.println("Sprinkler System Running");

  //   } else if (!status) {
  //     Serial.print("Shutting down Sprinkler system...");
  //     Latching.release();
  //     Latching.channelDirection(LATCHING_CMD_1, NEGATIVE); 
  //     Latching.channelDirection(LATCHING_CMD_2, NEGATIVE);
  //     Latching.channelDirection(LATCHING_CMD_4, NEGATIVE);
  //   // Test MR 3: Stop Inverter
  //     Latching.channelDirection(LATCHING_CMD_3, POSITIVE), Latching.latch();
  //     delay(250);
  //     Latching.release(); Latching.channelDirection(LATCHING_CMD_3, NEGATIVE);
  //     delay(250);
  //   Serial.println("Sprinkler System Stopped");
  //   }
// }

void inverter() {
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

void printLCD() {
  for (auto i = 0; i < 6; i++) {
    LCD.clear(), LCD.home();
    LCD.print(getAveragePressure());
    LCD.setCursor(0, 1);
    LCD.print(getBatPercent());
    delay(2000);
  }
}
