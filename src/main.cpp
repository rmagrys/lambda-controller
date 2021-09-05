#include <Arduino.h>
#include "cj125.h"
#include "regulator.h"


uint32_t programTime = 0;
uint32_t interval_time = 0;

uint16_t vspiCommand(uint16_t data);

void setup() {
  Serial.begin(115200);
  cj125PinInitialize();
  cj125SpiInitalize();
  cj125Startup();
  cj125Calibration();
}

void loop() {
  programTime = millis();

  if(programTime - interval_time > 1000)
  {
   
  }
}


