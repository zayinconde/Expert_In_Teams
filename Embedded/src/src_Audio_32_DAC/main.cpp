
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup() {
    auto cfg = i2s.defaultConfig();
    cfg.pin_bck = 18;
    cfg.pin_ws = 16;
    cfg.pin_data = 17;
    cfg.bits_per_sample = 16;
    cfg.sample_rate = 44100;
    i2s.begin(cfg);

    a2dp_sink.start("AI Audio DAC");
}

void loop() {
}