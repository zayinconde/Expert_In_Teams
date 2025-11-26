#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Setup started");
}

void loop() {
  Serial.println("Loop running");
  delay(1000);
}