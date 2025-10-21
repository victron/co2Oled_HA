#include "heater.h"

// ============================================================
// КОЕФІЦІЄНТИ ДЛЯ ESP8266 - ПОЛІНОМ 3-ГО СТУПЕНЯ
// ============================================================

// float a0 = 5.6283822058;
// float a1 = -1.4457510413e-01;
// float a2 = 5.1145811076e-04;
// float a3 = -2.8784432962e-07;

// Код для ESP8266:
// ------------------------------------------------------------

// float adc_to_temp(int adc) {
//     float x = (float)adc;
//     float x2 = x * x;
//     float x3 = x2 * x;

//     return a0 + a1*x + a2*x2 + a3*x3;
// }

// ============================================================
// КОЕФІЦІЄНТИ ДЛЯ ESP8266 - ПОЛІНОМ 4-ГО СТУПЕНЯ
// ============================================================

// float a0 = 456.2409054740;
// float a1 = -3.1286816344e+00;
// float a2 = 7.7992566178e-03;
// float a3 = -8.0640401703e-06;
// float a4 = 3.0590941788e-09;

// Конвертація ADC → температура
float getTemperatureFromADC(int adcValue) {
  // Перевірка на небезпечні зони
  if(adcValue < ADC_MIN_SAFE || adcValue > ADC_MAX_SAFE) {
    return 1000.0f;
  }

  // Поліном 4-го ступеня: T = a0 + a1*x + a2*x² + a3*x³ + a4*x⁴
  float x = (float)adcValue;
  float x2 = x * x;
  float x3 = x2 * x;
  float x4 = x2 * x2;

  return a0 + a1 * x + a2 * x2 + a3 * x3 + a4 * x4;
}

// Функція для зчитування температури
// З примітивним усередненням для покращення якості
float readTemperature(int samples) {
  long sum = 0;

  for(int i = 0; i < samples; i++) {
    sum += analogRead(THERMISTOR_PIN);
    delayMicroseconds(200);  // Компроміс швидкості та якості
  }
  // Загальний час: ~4-5 мс (дуже швидко!)

  return getTemperatureFromADC(sum / samples);
}

// Стани терморегулятора
enum ThermoState {
  IDLE,     // Початковий стан
  HEATING,  // Активний нагрів
  COOLING,  // Очікування охолодження
  SETTING   // Режим налаштування temperature
};

// State Machine - чиста логіка без таймерів
void updateThermostat() {
  // Читаємо поточну температуру
  currentTemp = readTemperature();

  switch(currentState) {
    case IDLE:
      relayState = false;

      if(currentTemp < targetTemp - HYSTERESIS) {
        currentState = HEATING;
      } else if(currentTemp > targetTemp + HYSTERESIS) {
        currentState = COOLING;
      }
      break;

    case HEATING:
      relayState = true;

      if(currentTemp >= targetTemp + HYSTERESIS) {
        currentState = COOLING;
      }
      break;

    case COOLING:
      relayState = false;

      if(currentTemp <= targetTemp - HYSTERESIS) {
        currentState = HEATING;
      }
      break;

    case SETTING:
      // Реле не змінюємо під час налаштування
      break;
  }

  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
}

// Створюємо екземпляри кнопок
button btnUp(BUTTON_UP);  // Твої піни
button btnDown(BUTTON_DOWN);

void handleButtons() {
  if(btnUp.click()) {
    targetTemp += 0.5;
    if(targetTemp > 35.0) targetTemp = 35.0;  // Максимум
    currentState = SETTING;
    lastButtonPress = millis();
  }

  if(btnDown.click()) {
    targetTemp -= 0.5;
    if(targetTemp < 5.0) targetTemp = 5.0;  // Мінімум
    currentState = SETTING;
    lastButtonPress = millis();
  }

  // Автовихід з режиму налаштування
  if(currentState == SETTING && millis() - lastButtonPress >= SETTING_TIMEOUT) {
    currentState = IDLE;
  }
}