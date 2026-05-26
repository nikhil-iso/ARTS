#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

// ARTS I2C bus pins on Teensy 4.1.
const int I2C_SDA_PIN = 18;
const int I2C_SCL_PIN = 19;

// You confirmed this BMP280 module is set to address 0x77.
const byte BMP280_ADDRESS_USED = 0x77;

Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS BMP280 pressure and temperature test");
  Serial.println("Expected I2C address: 0x77");

  // The board has no external I2C pull-up resistors, so enable weak internal
  // pull-ups before starting the I2C bus.
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(100000);

  if (!bmp.begin(BMP280_ADDRESS_USED)) {
    Serial.println("BMP280 was not found at 0x77.");
    Serial.print("Sensor ID read as: 0x");
    Serial.println(bmp.sensorID(), HEX);
    Serial.println("Check 3.3V, GND, SDA pin 18, SCL pin 19, and the SDO/address setting.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("BMP280 connected.");

  // Basic normal-mode settings from the Adafruit example.
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
}

void loop() {
  float tempC = bmp.readTemperature();
  float pressurePa = bmp.readPressure();
  float altitudeM = bmp.readAltitude(1013.25);

  Serial.print("BMP280 temperature: ");
  Serial.print(tempC, 2);
  Serial.print(" C  pressure: ");
  Serial.print(pressurePa, 0);
  Serial.print(" Pa  approx altitude: ");
  Serial.print(altitudeM, 1);
  Serial.println(" m");

  delay(1000);
}
