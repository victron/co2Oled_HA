#include "heater.h"

#define THERMISTOR_PIN 0  // Пін, до якого підключено термістор (аналоговий пін A0)
#define SERIES_RESISTOR 10000.0 // Опір резистора в дільнику напруги (наприклад, 10K)

// Константи для термістора
const float THERMISTOR_NOMINAL_RESISTANCE = 10000.0;  // Номінальний опір при 25°C (10K Ohm)
const float THERMISTOR_B_VALUE = 3950.0;              // B value термістора (3950K)
const float TEMPERATURE_NOMINAL = 25.0;               // Номінальна температура (25°C)
const float ESP8266_MAX_VOLTAGE = 1.0;                // Максимальна вхідна напруга на АЦП ESP8266
const float ESP8266_ADC_RESOLUTION = 1023.0;          // Роздільна здатність АЦП ESP8266

float readTemperature() {
  // Зчитуємо аналогове значення з піна термістора
  int sensorValue = analogRead(THERMISTOR_PIN);

  // Перетворюємо аналогове значення в напругу (враховуючи дільник напруги ESP8266)
  float voltage = sensorValue * (ESP8266_MAX_VOLTAGE / ESP8266_ADC_RESOLUTION);

  // Обчислюємо опір термістора
  // voltage = R1 / (R1 + Rt) * Vcc
  // Rt = R1 * (Vcc / voltage - 1)
  float resistance = SERIES_RESISTOR * ((3.3 / voltage) - 1.0);

  // Обчислюємо температуру за допомогою рівняння Steinhart-Hart
  float steinhart;
  steinhart = resistance / THERMISTOR_NOMINAL_RESISTANCE;  // (R/Ro)
  steinhart = log(steinhart);                              // ln(R/Ro)
  steinhart /= THERMISTOR_B_VALUE;                         // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);       // + (1/To)
  steinhart = 1.0 / steinhart;                             // Invert
  steinhart -= 273.15;                                     // convert to C

  return steinhart;
}
