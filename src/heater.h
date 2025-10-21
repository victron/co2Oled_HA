// thermistor.h
#ifndef THERMISTOR_H
#define THERMISTOR_H

#include <Arduino.h>

float readTemperature(int samples = 20);
float getTemperatureFromADC(int adcValue);

#endif