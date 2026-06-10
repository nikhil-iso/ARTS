#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// Teensy 4.1 I2C pins used by the ARTS board.
const int I2C_SDA_PIN = 18;
const int I2C_SCL_PIN = 19;

// Most MPU6050 breakout boards use 0x68. If AD0 is tied high, use 0x69.
const byte MPU6050_ADDRESS = 0x68;

void writeRegister(byte registerAddress, byte value) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(registerAddress);
  Wire.write(value);
  Wire.endTransmission();
}

byte readRegister(byte registerAddress) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(registerAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDRESS, (byte)1);

  if (Wire.available()) {
    return Wire.read();
  }

  return 0;
}

int16_t makeSignedWord(byte highByte, byte lowByte) {
  return (int16_t)((highByte << 8) | lowByte);
}

bool readMpu6050Raw(int16_t &rawAx, int16_t &rawAy, int16_t &rawAz,
                    int16_t &rawTemp, int16_t &rawGx,
                    int16_t &rawGy, int16_t &rawGz) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(MPU6050_ADDRESS, (byte)14) != 14) {
    return false;
  }

  rawAx = makeSignedWord(Wire.read(), Wire.read());
  rawAy = makeSignedWord(Wire.read(), Wire.read());
  rawAz = makeSignedWord(Wire.read(), Wire.read());
  rawTemp = makeSignedWord(Wire.read(), Wire.read());
  rawGx = makeSignedWord(Wire.read(), Wire.read());
  rawGy = makeSignedWord(Wire.read(), Wire.read());
  rawGz = makeSignedWord(Wire.read(), Wire.read());

  return true;
}

void setupMpu6050() {
  writeRegister(0x6B, 0x00); // Wake up the MPU6050.
  writeRegister(0x1C, 0x00); // Accelerometer range: +/- 2 g.
  writeRegister(0x1B, 0x00); // Gyro range: +/- 250 deg/s.
  delay(100);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS MPU6050 basic test");

  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(100000);

  byte whoAmI = readRegister(0x75);
  Serial.print("WHO_AM_I register: 0x");
  Serial.println(whoAmI, HEX);

  if (whoAmI != 0x68) {
    Serial.println("MPU6050 did not answer as expected.");
    Serial.println("Check 3.3V, GND, SDA pin 18, SCL pin 19, and address 0x68.");
    while (true) {
      delay(1000);
    }
  }

  setupMpu6050();
  Serial.println("MPU6050 connected.");
  Serial.println("Printing accel in g, gyro in deg/s, and basic pitch/roll.");
}

void loop() {
  int16_t rawAx, rawAy, rawAz, rawTemp, rawGx, rawGy, rawGz;

  if (!readMpu6050Raw(rawAx, rawAy, rawAz, rawTemp, rawGx, rawGy, rawGz)) {
    Serial.println("Could not read MPU6050.");
    delay(1000);
    return;
  }

  float ax = rawAx / 16384.0;
  float ay = rawAy / 16384.0;
  float az = rawAz / 16384.0;

  float gx = rawGx / 131.0;
  float gy = rawGy / 131.0;
  float gz = rawGz / 131.0;

  float tempC = (rawTemp / 340.0) + 36.53;

  float pitchDeg = atan2(-ax, sqrt((ay * ay) + (az * az))) * RAD_TO_DEG;
  float rollDeg = atan2(ay, az) * RAD_TO_DEG;

  Serial.print("Accel g  X:");
  Serial.print(ax, 3);
  Serial.print("  Y:");
  Serial.print(ay, 3);
  Serial.print("  Z:");
  Serial.print(az, 3);

  Serial.print("   Gyro deg/s  X:");
  Serial.print(gx, 2);
  Serial.print("  Y:");
  Serial.print(gy, 2);
  Serial.print("  Z:");
  Serial.print(gz, 2);

  Serial.print("   Pitch:");
  Serial.print(pitchDeg, 1);
  Serial.print("  Roll:");
  Serial.print(rollDeg, 1);

  Serial.print("   Temp:");
  Serial.print(tempC, 1);
  Serial.println(" C");

  delay(250);
}
