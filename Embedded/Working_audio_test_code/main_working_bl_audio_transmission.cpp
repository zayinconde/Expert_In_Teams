#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

AnalogAudioStream out;
BluetoothA2DPSink a2dp_sink(out);

void setup() {
    Serial.begin(115200);

    // configure analog audio output
    auto cfg = out.defaultConfig(TX_MODE);
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    cfg.channels = 2;
    out.begin(cfg);

    // start bluetooth a2dp sink
    a2dp_sink.start("AI Deputy"); // also device name 
}

void loop() {
}