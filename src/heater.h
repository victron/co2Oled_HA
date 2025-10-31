// thermistor.h
#ifndef THERMISTOR_H
#define THERMISTOR_H

#include <Arduino.h>

#include "button.h"
#define THERMISTOR_PIN 0  // Пін, до якого підключено термістор (аналоговий пін A0)
#define RELAY_PIN 5
#define RELEY_ON HIGH
#define RELEY_OFF LOW
#define BUTTON_DOWN 4
#define BUTTON_UP 13

// Безпечний діапазон ADC
#define ADC_MIN_SAFE 430
#define ADC_MAX_SAFE 850

// Коефіцієнти полінома 4-го ступеня
const float a0 = 456.2409054740;
const float a1 = -3.1286816344e+00;
const float a2 = 7.7992566178e-03;
const float a3 = -8.0640401703e-06;
const float a4 = 3.0590941788e-09;
const float HYSTERESIS = 2.0f;
const float maxTemp = 60.0f;
const float minTemp = 15.0f;

// Стани терморегулятора
enum class ThermoState {
  INIT,
  HEATING,
  COOLING,
  OFF
};

// ОГОЛОШЕННЯ (extern) - кажемо що вони існують десь
extern ThermoState heaterState;
extern float TempTarget;
extern float TempCurrent;
extern bool relayState;
extern const float HYSTERESIS;

extern unsigned long lastButtonPress;
extern const unsigned long SETTING_TIMEOUT;

float readTemperature(int samples = 20);
float getTemperatureFromADC(int adcValue);
void updateThermostat(float currentTemp);
void handleButtons();

#endif