#include <Arduino.h>
#include <driver/dac.h>
#include <math.h>


const int sampleRate = 20000; // 20 kHz sample rate
const int frequency = 1000;   // 1 kHz tone
const float pi = 3.14159265;





void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dac_output_enable(DAC_CHANNEL_1); // GPIO25
  dac_output_enable(DAC_CHANNEL_2); // GPIO26


    // Generate one second of sine wave
    for (int i = 0; i < sampleRate; i++) {
      float t = (float)i / sampleRate;
      uint8_t value = (sin(2 * pi * frequency * t) * 127) + 128; // 0–255
      dac_output_voltage(DAC_CHANNEL_1, value);
      dac_output_voltage(DAC_CHANNEL_2, value);
      delayMicroseconds(1000000 / sampleRate);
    }
}

void loop() {

}

// put function definitions here:
//int myFunction(int x, int y) {
//return x + y;
//}