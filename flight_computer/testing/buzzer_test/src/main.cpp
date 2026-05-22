#include <Arduino.h>

// The schematic connects the buzzer circuit to Teensy digital pin 5.
const int BUZZER_PIN = 5;

// These values keep the test easy to change and easy to explain.
const int BEEP_FREQUENCY_HZ = 2000;
const int BEEP_DURATION_MS = 100;
const int TIME_BETWEEN_BEEPS_MS = 2000;

void setup() {
  // Serial lets us see that the Teensy is running the uploaded test code.
  Serial.begin(115200);
  delay(1000);

  Serial.println("ARTS buzzer hardware test");
  Serial.println("The buzzer should beep once now, then once every 2 seconds.");

  // Set the buzzer pin as an output before using it.
  pinMode(BUZZER_PIN, OUTPUT);

  // tone(pin, frequency, duration) creates the sound on a passive buzzer.
  tone(BUZZER_PIN, BEEP_FREQUENCY_HZ, BEEP_DURATION_MS);
}

void loop() {
  // Wait between beeps so each beep is easy to hear.
  delay(TIME_BETWEEN_BEEPS_MS);

  Serial.print("Beep at ");
  Serial.print(millis());
  Serial.println(" ms");

  tone(BUZZER_PIN, BEEP_FREQUENCY_HZ, BEEP_DURATION_MS);
}
