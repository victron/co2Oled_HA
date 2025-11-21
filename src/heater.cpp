#include "heater.h"

#include "oled.h"

// ВИЗНАЧЕННЯ глобальних змінних - тільки тут, один раз!
ThermoState heaterState = ThermoState::INIT;
float TempTarget = 15.0f;
float TempCurrent = 1001.0f;
volatile bool relayState = false;

unsigned long lastButtonPress = 0;
const unsigned long SETTING_TIMEOUT = 10000;

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
// ДЛЯ ZERO-CROSSING
volatile bool pendingRelayState = false;
volatile bool relayChangeRequested = false;
volatile uint32_t lastZeroCrossCycles = 0;
volatile bool zeroCrossFlag = false;

IRAM_ATTR void zeroCrossISR() {
   // дуже коротко — записати мітку і встановити флаг
   lastZeroCrossCycles = ESP.getCycleCount();
   zeroCrossFlag = true;
 }

// працюємо з HALF-PERIOD (100Hz)
static uint32_t lastCrossTime = 0;
static uint32_t avgPeriod = 10000;  // старт: 10 ms = half-period при 50Hz

// new thresholds
static const uint32_t MIN_PERIOD_US = 10000 - 2000;  // мало
static const uint32_t MAX_PERIOD_US = 10000 + 2000;  // занадто довго (пропуск)
static const uint32_t MAX_JITTER_US = 1000;          // ±1 ms допустимий джитер
static const uint8_t OK_THRESHOLD = 8;               // скільки валідних імпульсів треба
static const uint8_t MAX_NOISE = 5;                  // скільки шумових імпульсів поспіль дозволено

static uint8_t okCounter = 0;
static uint8_t noiseCounter = 0;
static bool synced = false;

inline uint32_t cyclesToMicros(uint32_t c) {
  return c / ESP.getCpuFreqMHz();  // 80MHz → ділити на 80
}

inline void updateAveragePeriod(uint32_t p) {
  avgPeriod = (avgPeriod * 7 + p) / 8;  // IIR фільтр
}

// ----------------------------------------------------------------------
// FSM: фільтрація zero-cross + sync
// ----------------------------------------------------------------------
static uint32_t lastValidCrossCycles = 0;
void handleZeroCrossFSM() {
  if(!zeroCrossFlag) return;
  // скидаємо флаг тут (обробка в loop)
  zeroCrossFlag = false;

  // зчитати volatile
  uint32_t irqCycles = lastZeroCrossCycles;
  static uint32_t prevIrqCycles = 0;
  if(prevIrqCycles == 0) {
    prevIrqCycles = irqCycles;
    return;
  }

  uint32_t dtCycles = irqCycles - prevIrqCycles;
  prevIrqCycles = irqCycles;
  uint32_t period_us = cyclesToMicros(dtCycles);

  // Debug вивід
  static unsigned long lastDebugPrint = 0;
  if(millis() - lastDebugPrint > 2000) {
    lastDebugPrint = millis();
    Serial.print("Period: ");
    Serial.print(period_us);
    Serial.print(" µs | Synced: ");
    Serial.print(synced);
    Serial.print(" | OK: ");
    Serial.print(okCounter);
    Serial.print(" | Noise: ");
    Serial.println(noiseCounter);
  }

  bool validRange = (period_us >= MIN_PERIOD_US && period_us <= MAX_PERIOD_US);
  uint32_t diff = (period_us > avgPeriod) ? (period_us - avgPeriod) : (avgPeriod - period_us);
  bool validJitter = (diff < MAX_JITTER_US);
  bool okPulse = validRange && validJitter;

  if(okPulse) {
    okCounter++;
    noiseCounter = 0;
    updateAveragePeriod(period_us);
    lastValidCrossCycles = irqCycles;  // використовуємо мітку переривання

    if(!synced && okCounter >= OK_THRESHOLD) {
      synced = true;
      Serial.println("✓ Zero-cross SYNCED");
    }
  } else {
    Serial.print("Invalid pulse: ");
    Serial.print(period_us);
    Serial.print(" µs (range: ");
    Serial.print(validRange);
    Serial.print(", jitter: ");
    Serial.print(validJitter);
    Serial.println(")");

    noiseCounter++;
    okCounter = 0;

    if(synced && noiseCounter >= MAX_NOISE) {
      synced = false;
      Serial.println("✗ Zero-cross LOST");
    }
  }

  if(lastValidCrossCycles > 0) {
    uint32_t timeSinceLastValid = cyclesToMicros(irqCycles - lastValidCrossCycles);
    if(timeSinceLastValid > 120000) {
      synced = false;
      okCounter = 0;
      noiseCounter = 0;
      Serial.println("✗ No valid cross for 120ms - reset sync");
    }
  }
}

// ============ СПРОЩЕНИЙ scheduler ============
void handleZeroCrossScheduler() {
  if(!synced) return;
  if(!relayChangeRequested) return;

  // Перемикаємо ОДРАЗУ після синхронізації zero-cross
  // Не чекаємо "ідеального" моменту - він ніколи не настане
  digitalWrite(RELAY_PIN, pendingRelayState ? RELEY_ON : RELEY_OFF);

  relayState = pendingRelayState;
  relayChangeRequested = false;

  Serial.print("Relay switched to: ");
  Serial.println(relayState ? "ON" : "OFF");
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