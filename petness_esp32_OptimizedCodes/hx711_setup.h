#ifndef HX711_SETUP_H
#define HX711_SETUP_H

#include <Arduino.h>
#include <HX711_ADC.h>

// Declaration of calibrationValue as extern
extern float calibrationValue;

// Declaration of LoadCell as extern
extern HX711_ADC LoadCell;

void setupHX711();

#endif
