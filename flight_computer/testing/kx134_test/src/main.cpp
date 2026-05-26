#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_KX13X.h>

// ARTS I2C bus pins on Teensy 4.1.
const int I2C_SDA_PIN = 18;
const int I2C_SCL_PIN = 19;

// Your I2C scan found the KX134/KX13X at 0x1F.
// The SparkFun library uses the normal KX13X I2C address by default.
const byte KX134_EXPECTED_ADDRESS = 0x1F;

SparkFun_KX134 accelerometer;
outputData accelData;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS KX134 acceleration test");
  Serial.println("Expected I2C address: 0x1F");

  // The board has no external I2C pull-up resistors, so enable weak internal
  // pull-ups before starting the I2C bus.
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(100000);

  if (!accelerometer.begin()) {
    Serial.println("KX134 was not found.");
    Serial.println("Check 3.3V, GND, SDA pin 18, SCL pin 19, and address 0x1F.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("KX134 connected.");

  if (accelerometer.softwareReset()) {
    Serial.println("KX134 reset complete.");
  }

  delay(5);

  // Many KX134 settings should be changed while the accelerometer is disabled.
  accelerometer.enableAccel(false);
  accelerometer.setRange(SFE_KX134_RANGE16G);
  accelerometer.enableDataEngine();
  accelerometer.enableAccel();

  Serial.println("Reading acceleration in g.");
}

void loop() {
  accelData = accelerometer.getAccelData();

  float totalG = sqrt((accelData.xData * accelData.xData) +
                      (accelData.yData * accelData.yData) +
                      (accelData.zData * accelData.zData));

  Serial.print("X: ");
  Serial.print(accelData.xData, 3);
  Serial.print(" g  Y: ");
  Serial.print(accelData.yData, 3);
  Serial.print(" g  Z: ");
  Serial.print(accelData.zData, 3);
  Serial.print(" g  total: ");
  Serial.print(totalG, 3);
  Serial.println(" g");

  delay(200);
}
