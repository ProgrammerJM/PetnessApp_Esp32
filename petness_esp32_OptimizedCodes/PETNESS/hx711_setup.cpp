#include "hx711_setup.h"

// Definition of calibrationValue
float calibrationValue = 22.88;

// Definition of LoadCell
HX711_ADC LoadCell(13, 27);

void setupHX711() {
  LoadCell.begin();

  Serial.println("Load Cell Starting..."); 

  unsigned long stabilizingtime = 1000; 
  boolean _tare = true; 
  LoadCell.start(stabilizingtime, _tare);
  
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Load Cell Startup Timeout, check MCU>HX711 wiring and pin designations");
  } else {
    LoadCell.setCalFactor(calibrationValue); Serial.println("Load Cell Startup is complete");
  } 

  while (!LoadCell.update());
    Serial.print("Calibration value: "); Serial.println(LoadCell.getCalFactor());
    if (LoadCell.getSPS() < 7) {      
      Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
    } else if (LoadCell.getSPS() > 100) {  
      Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
    }
}
