#include <Arduino.h>
#include <Wire.h>
#include <SparkFunTMP102.h>

// ARTS I2C bus pins on Teensy 4.1.
const int I2C_SDA_PIN = 18;
const int I2C_SCL_PIN = 19;

// Your I2C scan found the TMP102 at 0x48.
const byte TMP102_ADDRESS = 0x48;

TMP102 temperatureSensor;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS TMP102 temperature test");
  Serial.println("Expected I2C address: 0x48");

  // The board has no external I2C pull-up resistors, so enable weak internal
  // pull-ups before starting the I2C bus.
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(100000);

  if (!temperatureSensor.begin(TMP102_ADDRESS, Wire)) {
    Serial.println("TMP102 was not found.");
    Serial.println("Check 3.3V, GND, SDA pin 18, SCL pin 19, and address 0x48.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("TMP102 connected.");

  // 1 Hz is simple and stable for a first temperature test.
  temperatureSensor.setConversionRate(1);
}

void loop() {
  float tempC = temperatureSensor.readTempC();
  float tempF = temperatureSensor.readTempF();

  Serial.print("TMP102 temperature: ");
  Serial.print(tempC, 2);
  Serial.print(" C  /  ");
  Serial.print(tempF, 2);
  Serial.println(" F");

  delay(1000);
}
