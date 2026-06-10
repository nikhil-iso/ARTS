#include <Arduino.h>
#include <Wire.h>
#include <math.h>

const int I2C_SDA_PIN = 18;
const int I2C_SCL_PIN = 19;
const byte MPU6050_ADDRESS = 0x68;

const unsigned long OUTPUT_INTERVAL_MS = 50;
const float COMPLEMENTARY_ALPHA = 0.98;
const int GYRO_CALIBRATION_SAMPLES = 500;

float pitchDeg = 0.0;
float rollDeg = 0.0;
float yawDeg = 0.0;

float gyroXOffset = 0.0;
float gyroYOffset = 0.0;
float gyroZOffset = 0.0;

unsigned long lastImuTime = 0;
unsigned long lastOutputTime = 0;

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
                    int16_t &rawGx, int16_t &rawGy, int16_t &rawGz) {
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

  // Skip temperature for the visualizer.
  Wire.read();
  Wire.read();

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

void calibrateGyro() {
  long gxSum = 0;
  long gySum = 0;
  long gzSum = 0;
  int samplesRead = 0;

  Serial.println("Keep the rocket still. Calibrating gyro...");

  while (samplesRead < GYRO_CALIBRATION_SAMPLES) {
    int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz;

    if (readMpu6050Raw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz)) {
      gxSum += rawGx;
      gySum += rawGy;
      gzSum += rawGz;
      samplesRead++;
    }

    delay(3);
  }

  gyroXOffset = (gxSum / (float)samplesRead) / 131.0;
  gyroYOffset = (gySum / (float)samplesRead) / 131.0;
  gyroZOffset = (gzSum / (float)samplesRead) / 131.0;

  Serial.println("Gyro calibration complete.");
}

void setStartingAngleFromAccelerometer() {
  int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz;
  if (!readMpu6050Raw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz)) {
    return;
  }

  float ax = rawAx / 16384.0;
  float ay = rawAy / 16384.0;
  float az = rawAz / 16384.0;

  pitchDeg = atan2(-ax, sqrt((ay * ay) + (az * az))) * RAD_TO_DEG;
  rollDeg = atan2(ay, az) * RAD_TO_DEG;
  yawDeg = 0.0;
}

void updateOrientation(float &ax, float &ay, float &az,
                       float &gx, float &gy, float &gz) {
  int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz;

  if (!readMpu6050Raw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz)) {
    return;
  }

  ax = rawAx / 16384.0;
  ay = rawAy / 16384.0;
  az = rawAz / 16384.0;

  gx = (rawGx / 131.0) - gyroXOffset;
  gy = (rawGy / 131.0) - gyroYOffset;
  gz = (rawGz / 131.0) - gyroZOffset;

  float accelPitch = atan2(-ax, sqrt((ay * ay) + (az * az))) * RAD_TO_DEG;
  float accelRoll = atan2(ay, az) * RAD_TO_DEG;

  unsigned long now = millis();
  float dtSeconds = (now - lastImuTime) / 1000.0;
  lastImuTime = now;

  if (dtSeconds <= 0.0 || dtSeconds > 1.0) {
    pitchDeg = accelPitch;
    rollDeg = accelRoll;
    return;
  }

  pitchDeg = (COMPLEMENTARY_ALPHA * (pitchDeg + (gy * dtSeconds))) +
             ((1.0 - COMPLEMENTARY_ALPHA) * accelPitch);
  rollDeg = (COMPLEMENTARY_ALPHA * (rollDeg + (gx * dtSeconds))) +
            ((1.0 - COMPLEMENTARY_ALPHA) * accelRoll);

  // Yaw is gyro-only here. It is useful for short demos, but it will drift
  // because the MPU6050 does not include a magnetometer.
  yawDeg += gz * dtSeconds;
}

void printOrientationRow(float ax, float ay, float az,
                         float gx, float gy, float gz) {
  Serial.print("ORIENT,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(pitchDeg, 2);
  Serial.print(",");
  Serial.print(rollDeg, 2);
  Serial.print(",");
  Serial.print(yawDeg, 2);
  Serial.print(",");
  Serial.print(ax, 3);
  Serial.print(",");
  Serial.print(ay, 3);
  Serial.print(",");
  Serial.print(az, 3);
  Serial.print(",");
  Serial.print(gx, 2);
  Serial.print(",");
  Serial.print(gy, 2);
  Serial.print(",");
  Serial.println(gz, 2);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS MPU6050 rocket orientation streamer");

  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(100000);

  byte whoAmI = readRegister(0x75);
  if (whoAmI != 0x68) {
    Serial.print("MPU6050 not found. WHO_AM_I = 0x");
    Serial.println(whoAmI, HEX);
    while (true) {
      delay(1000);
    }
  }

  setupMpu6050();
  calibrateGyro();
  setStartingAngleFromAccelerometer();

  lastImuTime = millis();

  Serial.println("ORIENT,millis,pitch_deg,roll_deg,yaw_deg,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps");
}

void loop() {
  float ax = 0.0;
  float ay = 0.0;
  float az = 0.0;
  float gx = 0.0;
  float gy = 0.0;
  float gz = 0.0;

  updateOrientation(ax, ay, az, gx, gy, gz);

  if (millis() - lastOutputTime >= OUTPUT_INTERVAL_MS) {
    printOrientationRow(ax, ay, az, gx, gy, gz);
    lastOutputTime = millis();
  }
}
