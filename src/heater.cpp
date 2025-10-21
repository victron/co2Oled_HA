#include "heater.h"

#define THERMISTOR_PIN 0  // Пін, до якого підключено термістор (аналоговий пін A0)

// -------- B value Method Implementation --------
// #define SERIES_RESISTOR 10000.0  // Опір резистора в дільнику напруги (наприклад, 10K)

// // Константи для термістора
// const float THERMISTOR_NOMINAL_RESISTANCE = 10000.0;  // Номінальний опір при 25°C (10K Ohm)
// const float THERMISTOR_B_VALUE = 3950.0;              // B value термістора (3950K)
// const float TEMPERATURE_NOMINAL = 25.0;               // Номінальна температура (25°C)
// const float ESP8266_MAX_VOLTAGE = 1.0;                // Максимальна вхідна напруга на АЦП ESP8266
// const float ESP8266_ADC_RESOLUTION = 1023.0;          // Роздільна здатність АЦП ESP8266

// float readTemperature() {
//   // Зчитуємо аналогове значення з піна термістора
//   int sensorValue = analogRead(THERMISTOR_PIN);

//   // Перетворюємо аналогове значення в напругу (враховуючи дільник напруги ESP8266)
//   float voltage = sensorValue * (ESP8266_MAX_VOLTAGE / ESP8266_ADC_RESOLUTION);

//   // Обчислюємо опір термістора
//   // voltage = R1 / (R1 + Rt) * Vcc
//   // Rt = R1 * (Vcc / voltage - 1)
//   float resistance = SERIES_RESISTOR * ((3.3 / voltage) - 1.0);

//   // Обчислюємо температуру за допомогою рівняння Steinhart-Hart
//   float steinhart;
//   steinhart = resistance / THERMISTOR_NOMINAL_RESISTANCE;  // (R/Ro)
//   steinhart = log(steinhart);                              // ln(R/Ro)
//   steinhart /= THERMISTOR_B_VALUE;                         // 1/B * ln(R/Ro)
//   steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);       // + (1/To)
//   steinhart = 1.0 / steinhart;                             // Invert
//   steinhart -= 273.15;                                     // convert to C

//   return steinhart;
// }
// -------------------------------------------------

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

// Безпечний діапазон ADC
#define ADC_MIN_SAFE 430
#define ADC_MAX_SAFE 850

// Коефіцієнти полінома 4-го ступеня
const float a0 = 456.2409054740;
const float a1 = -3.1286816344e+00;
const float a2 = 7.7992566178e-03;
const float a3 = -8.0640401703e-06;
const float a4 = 3.0590941788e-09;

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