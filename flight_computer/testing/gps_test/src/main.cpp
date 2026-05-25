#include <Arduino.h>
#include <Adafruit_GPS.h>

// Teensy Serial1 uses pin 0 as RX1 and pin 1 as TX1.
// Wire GPS TX to Teensy pin 0, and GPS RX to Teensy pin 1.
const int GPS_BAUD_RATE = 9600;
const int USB_BAUD_RATE = 115200;

// The ARTS buzzer circuit is connected to Teensy digital pin 5.
const int BUZZER_PIN = 5;
const int FIX_BEEP_FREQUENCY_HZ = 2000;
const int FIX_BEEP_DURATION_MS = 300;

// Print a readable GPS summary every 2 seconds.
const unsigned long SUMMARY_INTERVAL_MS = 2000;

// If no GPS bytes arrive for 3 seconds, print a wiring/baud warning.
const unsigned long NO_DATA_WARNING_MS = 3000;

Adafruit_GPS GPS(&Serial1);

unsigned long lastSummaryTime = 0;
unsigned long lastGpsByteTime = 0;
unsigned long lastNoDataMessageTime = 0;
bool hadFix = false;

void printTwoDigits(int value) {
  if (value < 10) {
    Serial.print("0");
  }
  Serial.print(value);
}

void printGpsSummary() {
  Serial.print("Summary: fix=");
  if (GPS.fix) {
    Serial.print("YES");
  } else {
    Serial.print("NO");
  }

  Serial.print("  quality=");
  Serial.print((int)GPS.fixquality);

  Serial.print("  satellites=");
  Serial.print((int)GPS.satellites);

  Serial.print("  time_utc=");
  printTwoDigits(GPS.hour);
  Serial.print(":");
  printTwoDigits(GPS.minute);
  Serial.print(":");
  printTwoDigits(GPS.seconds);

  if (GPS.fix) {
    Serial.print("  lat=");
    Serial.print(GPS.latitudeDegrees, 6);
    Serial.print("  lon=");
    Serial.print(GPS.longitudeDegrees, 6);
    Serial.print("  altitude_m=");
    Serial.print(GPS.altitude);
  } else {
    Serial.print("  waiting_for_fix");
  }

  Serial.println();
}

void beepWhenFixIsFound() {
  // GPS.fix becomes true after the GPS has a valid satellite position fix.
  // The hadFix flag makes this beep happen only once.
  if (GPS.fix && !hadFix) {
    Serial.println("GPS FIX FOUND - beeping buzzer.");
    tone(BUZZER_PIN, FIX_BEEP_FREQUENCY_HZ, FIX_BEEP_DURATION_MS);
    hadFix = true;
  }

  if (!GPS.fix) {
    hadFix = false;
  }
}

void setup() {
  Serial.begin(USB_BAUD_RATE);

  // Give the USB Serial Monitor a short time to connect after upload/reset.
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  Serial.println("ARTS GPS UART test using the Adafruit GPS library");
  Serial.println("Wiring: GPS TX -> Teensy pin 0, GPS RX -> Teensy pin 1, plus 3.3V and GND.");
  Serial.println("Open Serial Monitor at 115200 baud.");
  Serial.println("Raw NMEA lines prove the GPS serial connection is working.");
  Serial.println("A fix may take several minutes, especially indoors or after first power-up.");

  pinMode(BUZZER_PIN, OUTPUT);

  GPS.begin(GPS_BAUD_RATE);

  // Ask the GPS to send RMC and GGA sentences once per second.
  // RMC gives time/date/status. GGA gives fix quality, satellites, and altitude.
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  lastGpsByteTime = millis();
}

void loop() {
  // This reads one character from the GPS each time through loop().
  // The library stores complete NMEA lines internally.
  char gpsChar = GPS.read();
  if (gpsChar != 0) {
    lastGpsByteTime = millis();
  }

  if (GPS.newNMEAreceived()) {
    char *nmea = GPS.lastNMEA();

    Serial.print("NMEA: ");
    Serial.print(nmea);

    if (!GPS.parse(nmea)) {
      Serial.println("Could not parse this NMEA sentence.");
    }

    beepWhenFixIsFound();
  }

  if (millis() - lastSummaryTime >= SUMMARY_INTERVAL_MS) {
    printGpsSummary();
    lastSummaryTime = millis();
  }

  if (millis() - lastGpsByteTime > NO_DATA_WARNING_MS &&
      millis() - lastNoDataMessageTime > NO_DATA_WARNING_MS) {
    Serial.println("No GPS serial data yet. Check GPS power, ground, GPS TX -> Teensy pin 0, and baud rate.");
    lastNoDataMessageTime = millis();
  }
}
