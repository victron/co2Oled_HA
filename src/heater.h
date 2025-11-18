// thermistor.h
#ifndef THERMISTOR_H
#define THERMISTOR_H

#include <Arduino.h>

#include "Pins.h"
#include "button.h"
#define RELEY_ON HIGH
#define RELEY_OFF LOW

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
extern volatile bool relayState;
extern const float HYSTERESIS;

extern unsigned long lastButtonPress;
extern const unsigned long SETTING_TIMEOUT;

// ДЛЯ ZERO-CROSSING
extern volatile bool pendingRelayState;       // Бажаний стан реле
extern volatile bool relayChangeRequested;    // Флаг що потрібна зміна
extern volatile unsigned long lastZeroCross;  // Для debounce

float readTemperature(int samples = 20);
float getTemperatureFromADC(int adcValue);
void handleThermostat(float currentTemp);
void handleButtons();
void IRAM_ATTR zeroCrossingISR();
void requestRelayChange(bool newState);
void handleZeroCrossFSM();
#endif