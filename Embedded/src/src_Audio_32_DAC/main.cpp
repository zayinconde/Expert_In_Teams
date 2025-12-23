#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);


void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting Bluetooth PCM5102A DAC");

    AudioInfo info(44100, 2, 16);

    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.sample_rate = info.sample_rate;
    cfg.channels = info.channels;
    cfg.bits_per_sample = info.bits_per_sample;

    cfg.pin_bck  = 18;
    cfg.pin_ws   = 16; 
    cfg.pin_data = 17;

    i2s.end();
    delay(50);
    i2s.begin(cfg);
    delay(50);

    a2dp_sink.set_volume(100); //127 = 100%, turned down a bit to avoid distortion
    a2dp_sink.start("AI Audio DAC");

    Serial.println("Bluetooth ready");
}

void loop() {
    delay(1000);
}
