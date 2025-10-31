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

#define LED 2
//-----------------  wagchdog
Ticker watchdogTicker;
unsigned long lastFeedTime = 0;
const unsigned long WATCHDOG_TIMEOUT = 30000;
//----------------------

bool connected = false;
bool haInitialized = false;  // Прапорець що HA ініціалізовано
unsigned long lastWiFiAttempt = 0;
unsigned long lastUpdatHeater = 0;
unsigned long lastUpdatHA = 0;
const unsigned long WIFI_RETRY_INTERVAL = 5000;
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

  sender->setState(TempTarget);  // report the selected option back to the HA panel
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
  Serial.begin(115200);
  while(!Serial) {
    delay(100);
  }

  watchdogTicker.attach(5, watchdogCallback);
  lastFeedTime = millis();

  // OLED used nonstandard SDA and SCL pins
  Wire.begin(D5, D6);

  // LED id
  pinMode(LED, OUTPUT);  // LED pin as output
  digitalWrite(LED, LOW);

  pinMode(RELAY_PIN, OUTPUT);

  setupWiFi();

  init_oled();
  init_sensor();

  mqtt.onConnected(onMqttConnected);
  mqtt.onDisconnected(onMqttDisconnected);
  device.setName(HOSTNAME);  // name shown in HA
  device.setSoftwareVersion("1.1.0");
  init_ha(client, device, mqtt, co2Sensor, tempSensor, humSensor);
  currentTempHA.setIcon("mdi:thermometer");
  currentTempHA.setName("blanket current");
  currentTempHA.setUnitOfMeasurement("°C");

  targetTempHA.setIcon("mdi:heating-coil");
  targetTempHA.setName("blanket target");
  targetTempHA.setUnitOfMeasurement("°C");
  targetTempHA.setDeviceClass("temperature");
  targetTempHA.setMin(15.0f);  // not variables
  targetTempHA.setMax(50.0f);
  targetTempHA.setMode(HANumber::ModeSlider);
  targetTempHA.setRetain(true);
  targetTempHA.onCommand(onNumberCommand);

  wifiRssi.setIcon("mdi:wifi");
  wifiRssi.setName("WIFI RSSI");
  wifiRssi.setUnitOfMeasurement("dBm");

  heaterOnHA.setIcon("mdi:toggle-switch");
  heaterOnHA.setName("heater blanket");

  switchHA.setIcon("mdi:toggle-switch-variant-off");
  switchHA.setName("Switch heater");

  // Ініціалізація OTA з паролем
  setupOTA(HOSTNAME, OTA_PASSWORD);
}

void loop() {
  if(shouldRestart) {  // at beginning of loop
    Serial.println("Перезавантаження через watchdog...");
    Serial.flush();  // Дочекатись поки Serial виведе
    delay(100);
    ESP.restart();
  }

  // ----------------- local -----------------------
  // КНОПКИ - викликаємо в кожному циклі для швидкого відгуку
  handleButtons();
  handle_oled(co2, temperature, humidity);

  if((millis() - lastUpdatHeater) > 10000) {
    lastUpdatHeater = millis();

    // Read Measurement
    readMeasurement(co2, temperature, humidity, isDataReady);
    TempCurrent = readTemperature();
    updateThermostat(TempCurrent);
  }

  // ------------ remote -----------------------
  if(!connected && WiFi.status() == WL_CONNECTED) {
    // not mqtt and connected to wifi
    Serial.println("WiFi OK, mqtt NOK");
    digitalWrite(LED, (millis() / 1000) % 2);
  }

  if(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, LOW);
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
    if(isDataReady) {
      co2Sensor.setValue(co2);
      tempSensor.setValue(temperature);
      humSensor.setValue(humidity);
      currentTempHA.setValue(TempCurrent);
    }
    wifiRssi.setValue(WiFi.RSSI());
    heaterOnHA.setState(relayState);
  }

  lastFeedTime = millis();  // should be at the end of loop
}
