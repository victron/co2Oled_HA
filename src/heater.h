// thermistor.h
#ifndef THERMISTOR_H
#define THERMISTOR_H

#include <Arduino.h>

#include "button.h"
#define THERMISTOR_PIN 0  // Пін, до якого підключено термістор (аналоговий пін A0)
#define RELAY_PIN 5
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

// Глобальні змінні стану
ThermoState currentState = IDLE;
float targetTemp = 22.0f;       // Цільова температура
float currentTemp = 1001.0f;    // Поточна температура
bool relayState = false;        // Стан реле (false = OFF, true = ON)
const float HYSTERESIS = 2.0f;  // Гістерезис для уникнення тріпання реле

// Для автоматичного виходу з SETTING
unsigned long lastButtonPress = 0;
const unsigned long SETTING_TIMEOUT = 3000;  // 3 секунди без натискань

float readTemperature(int samples = 20);
float getTemperatureFromADC(int adcValue);
void updateThermostat();
void handleButtons();

#endif