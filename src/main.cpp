#include <Arduino.h>
#include <ArduinoHA.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <Wire.h>

#include "OTAHandler.h"
#include "co2sensor.h"
#include "config.h"
#include "ha_functions.h"
#include "heater.h"
#include "oled.h"

//-----------------  wagchdog
Ticker watchdogTicker;
unsigned long lastFeedTime = 0;
const unsigned long WATCHDOG_TIMEOUT = 120000;
const unsigned long WIFI_RETRY_INTERVAL = 30000;
//----------------------

bool connected = false;
bool haInitialized = false;  // Прапорець що HA ініціалізовано

// timers values
unsigned long lastWiFiAttempt = 0;
unsigned long lastUpdatHeater = 0;
unsigned long lastUpdatHA = 0;

// Глобальна змінна - прапорець для restart
volatile bool shouldRestart = false;

// globals for sensor
uint16_t co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;
bool isDataReady = false;
float lastSentTargetTemp = 15.0f;

bool heaterOn = false;

// globals for HA
WiFiClient client;
HADevice device(HOSTNAME);  // унікальний ID пристрою mqtt
HAMqtt mqtt(client, device);
HASensorNumber co2Sensor("co2", HASensorNumber::PrecisionP0);
HASensorNumber tempSensor("temperature", HASensorNumber::PrecisionP2);
HASensorNumber humSensor("humididy", HASensorNumber::PrecisionP2);
HASensorNumber wifiRssi("wifiRssi", HASensorNumber::PrecisionP0);
HASensorNumber currentTempHA("currentTempHA", HASensorNumber::PrecisionP1);
HABinarySensor heaterOnHA("heater_onr");
HANumber targetTempHA("targetBlanket", HANumber::PrecisionP0);
String switchID = String(HOSTNAME) + "switch";
HASwitch switchHA(switchID.c_str());

void onMqttConnected() {
  // Please note that you need to subscribe topic each time the connection with the broker is acquired.
  // in this reason all below callbacks needed
  // mqtt.onMessage(onMqttMessage);
  // mqtt.onConnected(onMqttConnected);
  // mqtt.onDisconnected(onMqttDisconnected);
  // mqtt.onStateChanged(onMqttStateChanged);
  Serial.println("Connected to the broker! subscribing");

  digitalWrite(LED, HIGH);
  connected = true;
  // sunchronize HA with current states
  switchHA.setState((heaterState != ThermoState::OFF));
  targetTempHA.setState(TempTarget);
}

void onMqttDisconnected() {
  Serial.println("Disconnected from the broker!");
  connected = false;
}

void onNumberCommand(HANumeric number, HANumber* sender) {
  if(!number.isSet()) {
    // the reset command was send by Home Assistant
  } else {
    // you can do whatever you want with the number as follows:
    TempTarget = number.toFloat();
    // protector for overheating

    if(TempTarget > maxTemp) {
      TempTarget = maxTemp;
    }
  }

  sender->setState(TempTarget);     // report the selected option back to the HA panel
  lastSentTargetTemp = TempTarget;  // щоб не публікувати зайвий раз (щоб не зациклювало)
}

void onSwitchCommand(bool state, HASwitch* sender) {
  heaterState = (state ? ThermoState::INIT : ThermoState::OFF);
  handleThermostat(TempCurrent);  // immediately update relay state
  sender->setState(state);        // report state back to the Home Assistant
}

void setupWiFi() {
  Serial.print("connecting to ");
  Serial.println(WIFI_SSID);
  lastWiFiAttempt = millis();

  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void publishRetainedTargetTemp(float value) {
  // workaround to publish retained command topic
  // Формуємо command topic
  String cmdTopic = "aha/";

  cmdTopic += HOSTNAME;
  cmdTopic += "/targetBlanket/cmd_t";

  // Формуємо payload (HA очікує ціле число: 25.5 → "255")
  char payload[16];
  sprintf(payload, "%.0f", value);

  // Публікуємо з retained=true
  bool result = mqtt.publish(
      cmdTopic.c_str(),  // topic
      payload,           // payload
      true               // retained
  );

  Serial.print("Published retained command: ");
  Serial.print(payload);
  Serial.print(" -> ");
  Serial.println(result ? "OK" : "FAILED");
}

void watchdogCallback() {
  if(millis() - lastFeedTime > WATCHDOG_TIMEOUT) {
    Serial.println("WATCHDOG TIMEOUT!");
    shouldRestart = true;  // ← Встановити прапорець замість restart
  }
}

void setup() {
  // Serial.begin(115200);
  // while(!Serial) {
  //   delay(100);
  // }
  Serial.end();               // disable USB serial
  ESP.wdtDisable();           // вимкнути HW WDT перед налаштуванням
  ESP.wdtEnable(WDTO_120MS);  // Hardware WDT
  watchdogTicker.attach(5, watchdogCallback);
  lastFeedTime = millis();

  // OLED used nonstandard SDA and SCL pins
  Wire.begin(D5, D6);
  // Встановити timeout для clock stretching
  Wire.setClockStretchLimit(1500);  // мікросекунди

  // Знизити швидкість для стабільності
  Wire.setClock(50000);  // 50kHz замість 100kHz

  // LED id
  pinMode(LED, OUTPUT);  // LED pin as output
  digitalWrite(LED, LOW);
  pinMode(ZERO_DETECT, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELEY_OFF);
  attachInterrupt(digitalPinToInterrupt(ZERO_DETECT), zeroCrossISR, RISING);

  setupWiFi();

  init_oled();
  init_sensor();

  mqtt.onConnected(onMqttConnected);
  mqtt.onDisconnected(onMqttDisconnected);
  device.setName(HOSTNAME);  // name shown in HA
  device.setSoftwareVersion("1.1.0");
  delay(10000);  // wait for wiFi connection
  init_ha(client, device, mqtt, co2Sensor, tempSensor, humSensor);
  currentTempHA.setIcon("mdi:thermometer");
  currentTempHA.setName("blanket current");
  currentTempHA.setUnitOfMeasurement("°C");
  currentTempHA.setDeviceClass("temperature");  // to group on one graph in HA

  targetTempHA.setIcon("mdi:heating-coil");
  targetTempHA.setName("blanket target");
  targetTempHA.setUnitOfMeasurement("°C");
  targetTempHA.setDeviceClass("temperature");  // to group on one graph in HA
  targetTempHA.setMin(15.0f);                  // not variables
  targetTempHA.setMax(50.0f);
  targetTempHA.setMode(HANumber::ModeSlider);
  targetTempHA.setRetain(true);
  targetTempHA.onCommand(onNumberCommand);

  wifiRssi.setIcon("mdi:wifi");
  wifiRssi.setName("WIFI RSSI");
  wifiRssi.setUnitOfMeasurement("dBm");
  wifiRssi.setDeviceClass("signal_strength");

  heaterOnHA.setIcon("mdi:toggle-switch");
  heaterOnHA.setName("heater blanket");
  // TODO: with device class "switch" it's not shown in HA correctly
  // heaterOnHA.setDeviceClass("temperature");

  switchHA.setIcon("mdi:toggle-switch-variant-off");
  switchHA.setName("Switch heater");
  switchHA.setRetain(true);
  switchHA.onCommand(onSwitchCommand);

  // Ініціалізація OTA з паролем
  setupOTA(HOSTNAME, OTA_PASSWORD);
}

void loop() {
  // Обробка zero-cross: фільтрація (1×) і scheduler (частіше)
  handleZeroCrossFSM();        // фільтрує імпульси — один виклик достатній
  handleZeroCrossScheduler();  // перший виклик scheduler — ранній

  ESP.wdtFeed();  // годівля WDT якнайшвидше

  // Оновлюємо маркер активності одразу — щоб коротка відсутність WiFi не викликала рестарт
  lastFeedTime = millis();

  if(shouldRestart) ESP.restart();  // безпечний виклик рестарту

  // ----------------- local -----------------------
  // КНОПКИ - викликаємо в кожному циклі для швидкого відгуку
  handleButtons();
  handle_oled(co2, temperature, humidity);

  // Перед читанням сенсорів / оновленням термостата — ще раз визвати scheduler
  handleZeroCrossScheduler();

  if((millis() - lastUpdatHeater) > 10000) {
    lastUpdatHeater = millis();
    readMeasurement(co2, temperature, humidity, isDataReady);
    TempCurrent = readTemperature();
    handleThermostat(TempCurrent);
  }

  // ------------ remote -----------------------
  // Перед мережевими викликами — ще раз scheduler
  handleZeroCrossScheduler();

  if(!connected && WiFi.status() == WL_CONNECTED) {
    // not mqtt and connected to wifi
    Serial.println("WiFi OK, mqtt NOK");
    digitalWrite(LED, (millis() / 1000) % 2);
  }

  if(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, (millis() / 200) % 2);
    // Спроба підключення раз на 5 секунд
    if(millis() - lastWiFiAttempt >= WIFI_RETRY_INTERVAL) {
      lastWiFiAttempt = millis();
      setupWiFi();
    }
    return;  // не підключені до WiFi
  }

  mqtt.loop();
  ArduinoOTA.handle();

  if(abs(TempTarget - lastSentTargetTemp) > 0.1f) {  // it's critical to have same initial values of variables, to avoid unnecessary publish
    publishRetainedTargetTemp(TempTarget);
    lastSentTargetTemp = TempTarget;
    targetTempHA.setState(TempTarget);
  }

  if((millis() - lastUpdatHA) > 5000) {
    lastUpdatHA = millis();
    if(isDataReady) {
      co2Sensor.setValue(co2);
      tempSensor.setValue(temperature);
      humSensor.setValue(humidity);
    }
    currentTempHA.setValue(TempCurrent);
    // targetTempHA.setState(TempTarget);  // to keep all on one graph
    wifiRssi.setValue(WiFi.RSSI());
  }
  heaterOnHA.setState(relayState);

}
