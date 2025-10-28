#include <Arduino.h> 
#include <Wire.h> // for i2c
#include <Haptic_Driver.h> // for da7280 library

Haptic_Driver haptic;          // address fixed to 0x4A by this lib


//function for showing status messages
static void assertMsg(bool ok, const char* msg) {
  Serial.print(msg); 
  Serial.println(ok ? " OK" : " FAIL");
}


void setup(){
Serial.begin(115200);
delay(200);
Wire.begin(21, 22); // SDA, SCL
Wire.setClock(400000); // 400kHz I2C //DA7280 supports up to 1MHz

if(haptic.begin(Wire)){
    Serial.println("Haptic Driver Found!");
  } else {
    Serial.println("Haptic Driver Not Found!");
    while(1); // halt
  }

  haptic.setActuatorType(ERM_TYPE); // set motor type to ERM

  Serial.println("Setting I2C Operation");
  haptic.setOperationMode(DRO_MODE); // set to I2C control mode
  Serial.println("Ready boiiii!");

  

  haptic.enableFreqTrack(false); // disable freq track for coin ERM

  haptic.setActuatorNOMVolt(2.7f);
  haptic.setActuatorABSVolt(3.0f);
  haptic.setActuatorIMAX(0.10f);
  haptic.setActuatorImpedance(26.0f);

  haptic.enableRapidStop(true);
  haptic.enableAmpPid(true);


 


}

void loop() {
  // put your main code here, to run repeatedly:
  haptic.setVibrate(255); // max vibration
  Serial.println("Vibrating at max!");
  delay(1000);
  haptic.setVibrate(0); // stop vibration
  Serial.println("Stopping Vibration");
  delay(1000);

  Serial.print("Current mode: ");
  Serial.println(haptic.getOperationMode()); // if supported


}