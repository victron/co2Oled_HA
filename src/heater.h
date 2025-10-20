// thermistor.h
#ifndef THERMISTOR_H
#define THERMISTOR_H

#include <Arduino.h>

float readTemperature();
float getTemperatureFromADC(int adcValue);

#endif