#include <Arduino.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Starting Bluetooth DAC...");
    
    AudioInfo info(44100, 2, 16);
    
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = 18;
    cfg.pin_ws = 16;
    cfg.pin_data = 17;
    cfg.copyFrom(info);
    
    // Clean initialization
    i2s.end();
    delay(100);
    i2s.begin(cfg);
    delay(100);
    
    a2dp_sink.set_volume(200);
    a2dp_sink.start("AI Audio DAC");
    
    Serial.println("Bluetooth ready - connect your phone");
}

void loop() {
    delay(1000);
}