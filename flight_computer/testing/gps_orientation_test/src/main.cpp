#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <Adafruit_GPS.h>

// Hardware connections:
// GPS TX -> Teensy pin 0 RX1
// GPS RX -> Teensy pin 1 TX1
// MPU6050 SDA -> Teensy pin 18 SDA
// MPU6050 SCL -> Teensy pin 19 SCL

const int USB_BAUD_RATE = 115200;
const int GPS_BAUD_RATE = 9600;

const int I2C_SDA_PIN = 18;
const int I2C_SCL_PIN = 19;
const byte MPU6050_ADDRESS = 0x68;

const unsigned long OUTPUT_INTERVAL_MS = 1000;
const float COMPLEMENTARY_ALPHA = 0.98;

Adafruit_GPS GPS(&Serial1);

float pitchDeg = 0.0;
float rollDeg = 0.0;

unsigned long lastImuTime = 0;
unsigned long lastOutputTime = 0;

int latestGsvSatellitesVisible = 0;
int latestGsvSnrCount = 0;
float latestGsvAverageSnr = 0.0;
int latestGsvBestSnr = 0;

int activeGsvTotalMessages = 0;
int activeGsvSnrCount = 0;
int activeGsvSnrSum = 0;
int activeGsvBestSnr = 0;

void writeRegister(byte deviceAddress, byte registerAddress, byte value) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  Wire.write(value);
  Wire.endTransmission();
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

  if (Wire.requestFrom(MPU6050_ADDRESS, 14) != 14) {
    return false;
  }

  rawAx = makeSignedWord(Wire.read(), Wire.read());
  rawAy = makeSignedWord(Wire.read(), Wire.read());
  rawAz = makeSignedWord(Wire.read(), Wire.read());

  // Temperature bytes are present in the register block but not needed here.
  Wire.read();
  Wire.read();

  rawGx = makeSignedWord(Wire.read(), Wire.read());
  rawGy = makeSignedWord(Wire.read(), Wire.read());
  rawGz = makeSignedWord(Wire.read(), Wire.read());

  return true;
}

void setupMpu6050() {
  writeRegister(MPU6050_ADDRESS, 0x6B, 0x00); // Wake up the MPU6050.
  writeRegister(MPU6050_ADDRESS, 0x1C, 0x00); // Accel range: +/- 2 g.
  writeRegister(MPU6050_ADDRESS, 0x1B, 0x00); // Gyro range: +/- 250 deg/s.
  delay(100);
}

void updateOrientation() {
  int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz;
  if (!readMpu6050Raw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz)) {
    return;
  }

  float ax = rawAx / 16384.0;
  float ay = rawAy / 16384.0;
  float az = rawAz / 16384.0;

  float gx = rawGx / 131.0;
  float gy = rawGy / 131.0;

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
}

String getCsvField(const String &text, int fieldIndex) {
  int currentField = 0;
  int startIndex = 0;

  for (int i = 0; i <= text.length(); i++) {
    if (i == text.length() || text.charAt(i) == ',') {
      if (currentField == fieldIndex) {
        String field = text.substring(startIndex, i);
        int checksumIndex = field.indexOf('*');
        if (checksumIndex >= 0) {
          field = field.substring(0, checksumIndex);
        }
        return field;
      }
      currentField++;
      startIndex = i + 1;
    }
  }

  return "";
}

void parseGsvSentence(const String &nmea) {
  int totalMessages = getCsvField(nmea, 1).toInt();
  int messageNumber = getCsvField(nmea, 2).toInt();
  int satellitesVisible = getCsvField(nmea, 3).toInt();

  if (messageNumber == 1) {
    activeGsvTotalMessages = totalMessages;
    activeGsvSnrCount = 0;
    activeGsvSnrSum = 0;
    activeGsvBestSnr = 0;
  }

  // GSV repeats 4 fields per satellite: PRN, elevation, azimuth, SNR.
  for (int snrField = 7; snrField <= 19; snrField += 4) {
    String snrText = getCsvField(nmea, snrField);
    if (snrText.length() > 0) {
      int snr = snrText.toInt();
      activeGsvSnrSum += snr;
      activeGsvSnrCount++;

      if (snr > activeGsvBestSnr) {
        activeGsvBestSnr = snr;
      }
    }
  }

  if (totalMessages > 0 && messageNumber == activeGsvTotalMessages) {
    latestGsvSatellitesVisible = satellitesVisible;
    latestGsvSnrCount = activeGsvSnrCount;
    latestGsvBestSnr = activeGsvBestSnr;

    if (activeGsvSnrCount > 0) {
      latestGsvAverageSnr = (float)activeGsvSnrSum / activeGsvSnrCount;
    } else {
      latestGsvAverageSnr = 0.0;
    }
  }
}

void readGps() {
  GPS.read();

  if (GPS.newNMEAreceived()) {
    char *nmea = GPS.lastNMEA();
    String nmeaText = String(nmea);

    if (nmeaText.indexOf("GSV") == 3) {
      parseGsvSentence(nmeaText);
    }

    GPS.parse(nmea);
  }
}

void printCsvHeader() {
  Serial.println("DATA,millis,pitch_deg,roll_deg,tilt_deg,fix,fix_quality,sats_used,sats_visible,hdop,avg_snr,best_snr,snr_count,lat_deg,lon_deg,alt_m");
}

void printCsvRow() {
  float tiltDeg = sqrt((pitchDeg * pitchDeg) + (rollDeg * rollDeg));

  Serial.print("DATA,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(pitchDeg, 2);
  Serial.print(",");
  Serial.print(rollDeg, 2);
  Serial.print(",");
  Serial.print(tiltDeg, 2);
  Serial.print(",");
  Serial.print(GPS.fix ? 1 : 0);
  Serial.print(",");
  Serial.print((int)GPS.fixquality);
  Serial.print(",");
  Serial.print((int)GPS.satellites);
  Serial.print(",");
  Serial.print(latestGsvSatellitesVisible);
  Serial.print(",");
  Serial.print(GPS.HDOP);
  Serial.print(",");
  Serial.print(latestGsvAverageSnr, 1);
  Serial.print(",");
  Serial.print(latestGsvBestSnr);
  Serial.print(",");
  Serial.print(latestGsvSnrCount);
  Serial.print(",");

  if (GPS.fix) {
    Serial.print(GPS.latitudeDegrees, 7);
    Serial.print(",");
    Serial.print(GPS.longitudeDegrees, 7);
    Serial.print(",");
    Serial.print(GPS.altitude, 1);
  } else {
    Serial.print(",,");
  }

  Serial.println();
}

void setup() {
  Serial.begin(USB_BAUD_RATE);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS GPS antenna orientation logger");
  Serial.println("Hold each orientation for about 60 seconds for reliable data.");
  Serial.println("Run tools/collect_and_plot.py on the computer to save CSV and plot a hotspot map.");

  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(100000);
  setupMpu6050();

  int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz;
  if (readMpu6050Raw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz)) {
    float ax = rawAx / 16384.0;
    float ay = rawAy / 16384.0;
    float az = rawAz / 16384.0;
    pitchDeg = atan2(-ax, sqrt((ay * ay) + (az * az))) * RAD_TO_DEG;
    rollDeg = atan2(ay, az) * RAD_TO_DEG;
  }

  lastImuTime = millis();

  GPS.begin(GPS_BAUD_RATE);

  // RMC/GGA provide fix, time, position, satellites used, HDOP, and altitude.
  // GSV provides per-satellite signal strength, reported as SNR.
  // Enabling GSV means more serial text from the GPS, but it gives the best
  // orientation-quality signal for antenna testing.
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  printCsvHeader();
}

void loop() {
  readGps();
  updateOrientation();

  if (millis() - lastOutputTime >= OUTPUT_INTERVAL_MS) {
    printCsvRow();
    lastOutputTime = millis();
  }
}
