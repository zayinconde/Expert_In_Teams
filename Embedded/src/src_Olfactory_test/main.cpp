#include <Arduino.h>

// PWM settings
const int pwmPin = 32;       // GPIO32
const int pwmChannel = 0;    // LEDC channel (0–15 available)
const int pwmFreq = 110e3;  // 110 kHz
const int pwmResolution = 8; // bits (1–20, but higher res lowers max freq)

void setup() {
  // Configure PWM
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(pwmPin, pwmChannel);

  // Set duty cycle (50% here)
  int duty = (1 << pwmResolution) / 2; 
  ledcWrite(pwmChannel, duty);
}

void loop() {
  // Nothing needed — PWM runs in hardware
}