#include <Arduino.h>

const int pwmPin = 32;          // GPIO32
const int pwmChannel = 0;       // LEDC channel (0–15)
const int pwmResolution = 8;    // 8-bit resolution
const int pwmFrequency = 110000; // 110 kHz

void setup() {
  // Configure PWM channel
  ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
  // Attach channel to pin
  ledcAttachPin(pwmPin, pwmChannel);
  // Set duty cycle (0–255 for 8-bit)
  ledcWrite(pwmChannel, 128); // 50% duty cycle
}

void loop() {
  // PWM runs in hardware, no code needed here
}