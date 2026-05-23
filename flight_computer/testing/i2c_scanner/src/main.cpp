#include <Arduino.h>
#include <Wire.h>

// The ARTS schematic uses the default Teensy 4.1 I2C pins.
const int I2C_SDA_PIN = 18;
const int I2C_SCL_PIN = 19;

// Use 100 kHz for this first test. It is slower than 400 kHz, but easier on
// a new hand-soldered bus while checking connections.
const int I2C_CLOCK_HZ = 100000;

// Wait between scans so the Serial Monitor output is easy to read.
const int TIME_BETWEEN_SCANS_MS = 5000;

void printKnownDeviceName(byte address) {
  if (address == 0x48) {
    Serial.print("  TMP102 likely");
  } else if (address == 0x76 || address == 0x77) {
    Serial.print("  BMP280 likely");
  } else if (address == 0x1E || address == 0x1F) {
    Serial.print("  KX13X/KX134 possible");
  }
}

void scanI2CBus() {
  int devicesFound = 0;

  Serial.println();
  Serial.println("Scanning I2C bus...");

  // Most 7-bit I2C device addresses are between 0x08 and 0x77.
  for (byte address = 0x08; address <= 0x77; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found device at 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      printKnownDeviceName(address);
      Serial.println();

      devicesFound++;
    } else if (error == 4) {
      Serial.print("Unknown I2C error at 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  if (devicesFound == 0) {
    Serial.println("No I2C devices found.");
    Serial.println("Check 3.3V, GND, SDA pin 18, SCL pin 19, and solder joints.");
  } else {
    Serial.print("Scan complete. Devices found: ");
    Serial.println(devicesFound);
  }
}

void setup() {
  Serial.begin(115200);

  // Give the USB Serial Monitor a short time to connect after upload/reset.
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS I2C bus scanner");
  Serial.println("Expected devices include TMP102, BMP280, and KX13X/KX134.");

  // The design does not use external I2C pull-up resistors, so enable the
  // Teensy's weak internal pull-ups before starting Wire.
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(I2C_CLOCK_HZ);
}

void loop() {
  scanI2CBus();
  delay(TIME_BETWEEN_SCANS_MS);
}
