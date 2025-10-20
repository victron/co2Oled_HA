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

// -------- Lookup Table Method Implementation --------

// Константи для калібрування термістора (на основі history.csv)
const int NUM_POINTS = 40;  // Використовуємо 100 точок
// Масив значень ADC (монотонно зростає з температурою)
const int adcValues[NUM_POINTS] PROGMEM = {
    501, 502, 503, 504, 505, 506, 507, 508, 519, 533,
    544, 555, 567, 583, 596, 600, 616, 629, 639, 655,
    670, 683, 698, 718, 733, 743, 762, 778, 792, 802,
    817, 832, 847, 861, 875, 890, 904, 919, 933, 947};

// Масив відповідних температур (°C)
const float temperatures[NUM_POINTS] PROGMEM = {
    12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 22.0f, 24.0f, 26.0f, 28.0f, 30.0f,
    32.0f, 34.0f, 36.0f, 38.0f, 40.0f, 42.0f, 44.0f, 46.0f, 48.0f, 50.0f,
    52.0f, 54.0f, 56.0f, 58.0f, 60.0f, 62.0f, 64.0f, 66.0f, 68.0f, 70.0f,
    72.0f, 74.0f, 76.0f, 78.0f, 80.0f, 82.0f, 84.0f, 86.0f, 88.0f, 90.0f};

float getTemperatureFromADC(int adcValue) {
  // Обмеження adcValue в межах таблиці
  adcValue = std::max(adcValue, adcValues[0]);
  adcValue = std::min(adcValue, adcValues[NUM_POINTS - 1]);

  // Лінійний пошук та інтерполяція
  for(int i = 0; i < NUM_POINTS - 1; i++) {
    if(adcValue >= adcValues[i] && adcValue <= adcValues[i + 1]) {
      float fraction = (float)(adcValue - adcValues[i]) / (adcValues[i + 1] - adcValues[i]);
      return temperatures[i] + fraction * (temperatures[i + 1] - temperatures[i]);
    }
  }
  return -1000.0;  // Помилка
}

float readTemperature() {
  int sensorValue = analogRead(THERMISTOR_PIN);
  return getTemperatureFromADC(sensorValue);
}