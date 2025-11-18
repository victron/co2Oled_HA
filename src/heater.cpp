#include "heater.h"

#include "oled.h"

// ВИЗНАЧЕННЯ глобальних змінних - тільки тут, один раз!
ThermoState heaterState = ThermoState::INIT;
float TempTarget = 15.0f;
float TempCurrent = 1001.0f;
volatile bool relayState = false;

unsigned long lastButtonPress = 0;
const unsigned long SETTING_TIMEOUT = 10000;

// ДЛЯ ZERO-CROSSING
volatile bool pendingRelayState = false;
volatile bool relayChangeRequested = false;
volatile unsigned long lastZeroCross = 0;
volatile bool zeroCrossFlag = false;

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

  return getTemperatureFromADC(sum / samples);
}
/// ------------------- zero-crossing та реле ------------------- ///
// ISR - ОБРОБНИК ПЕРЕРИВАННЯ (максимально швидкий!)
void IRAM_ATTR zeroCrossingISR() {
  zeroCrossFlag = true;  // NOTHING MORE!
}

void handleZeroCrossFSM() {
  static unsigned long lastCrossTime = 0;
  static uint8_t noiseCounter = 0;

  if(!zeroCrossFlag) return;  // no event
  zeroCrossFlag = false;      // reset flag

  unsigned long now = micros();
  unsigned long diff = now - lastCrossTime;

  // -------------------------------------
  //  AUTO-RECOVERY & NOISE FILTER
  // -------------------------------------
  // нормальні інтервали: 5000–15000 µs (50 Hz мережева синусоїда)
  if(diff < 3000 || diff > 20000) {
    // Вважаємо це шумовим імпульсом
    noiseCounter++;

    if(noiseCounter >= 10) {
      // Якщо надто багато шуму — робимо автокорекцію
      noiseCounter = 0;
      lastCrossTime = now;
    }
    return;  // пропуск
  }

  // якщо ми тут → zero-cross реальний
  noiseCounter = 0;
  lastCrossTime = now;

  // -------------------------------------
  //     DELAYED RELAY SWITCHING HERE
  // -------------------------------------
  if(relayChangeRequested) {
    digitalWrite(RELAY_PIN, pendingRelayState ? RELEY_ON : RELEY_OFF);
    relayState = pendingRelayState;
    relayChangeRequested = false;
  }
}

//  ФУНКЦІЯ ДЛЯ ЗАПИТУ ЗМІНИ СТАНУ РЕЛЕ
void requestRelayChange(bool newState) {
  if(relayState == newState) {
    return;  // Стан не змінився - нічого не робимо
  }

  // Встановлюємо бажаний стан та флаг
  pendingRelayState = newState;
  relayChangeRequested = true;

  // ISR зробить фактичне перемикання при наступному zero-crossing
}

// State Machine - чиста логіка без таймерів
void handleThermostat(float TempCurrent) {
  switch(heaterState) {
    case ThermoState::INIT:
      if(TempCurrent <= TempTarget - HYSTERESIS) {
        heaterState = ThermoState::HEATING;
      }
      if(TempCurrent >= TempTarget + HYSTERESIS) {
        heaterState = ThermoState::COOLING;
      }
      break;

    case ThermoState::HEATING:
      requestRelayChange(true);

      if(TempCurrent >= TempTarget + HYSTERESIS) {
        heaterState = ThermoState::COOLING;
      }
      break;

    case ThermoState::COOLING:
      requestRelayChange(false);

      if(TempCurrent <= TempTarget - HYSTERESIS) {
        heaterState = ThermoState::HEATING;
      }
      break;

    case ThermoState::OFF:
      requestRelayChange(false);
      break;
  }
}

// Створюємо екземпляри кнопок
button btnUp(BUTTON_UP);  // Твої піни
button btnDown(BUTTON_DOWN);

void handleButtons() {
  if(btnUp.isPressed() && btnDown.isPressed()) {
    oledState = OledState::CO2_DISPLAY;
    lastButtonPress = millis();
    return;  // Виходимо, не обробляємо click
  }

  bool upClicked = btnUp.click();
  bool downClicked = btnDown.click();
  if(upClicked || downClicked) {
    if(oledState == OledState::OFF) {
      oledState = OledState::SET_TARGET;
      lastButtonPress = millis();
      return;  // Не змінюємо температуру
    }
  }

  if(upClicked) {
    // В режимі SETTING - змінюємо температуру
    TempTarget += 1.0;
    if(TempTarget > 40.0) TempTarget = 40.0;  // Максимум
    lastButtonPress = millis();
  }

  if(downClicked) {
    // В режимі SETTING - змінюємо температуру
    TempTarget -= 1.0;
    if(TempTarget < 5.0) TempTarget = 5.0;  // Мінімум
    lastButtonPress = millis();
  }

  // Автовихід з режиму налаштування
  if(oledState != OledState::OFF && millis() - lastButtonPress >= SETTING_TIMEOUT) {
    if(heaterState != ThermoState::OFF) heaterState = ThermoState::INIT;  // if heater was ON, return to INIT state
    oledState = OledState::OFF;
  }
}