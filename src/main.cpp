#include <Arduino.h>
#include <ArduinoHA.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

#include "OTAHandler.h"
#include "co2sensor.h"
#include "config.h"
#include "ha_functions.h"
#include "heater.h"
#include "oled.h"

#define LED 2

bool connected = false;
bool haInitialized = false;  // Прапорець що HA ініціалізовано
unsigned long lastWiFiAttempt = 0;
const unsigned long WIFI_RETRY_INTERVAL = 5000;
// globals for sensor
uint16_t co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;
bool isDataReady = false;

bool heaterOn = false;

// globals for HA
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);
HASensorNumber co2Sensor("co2", HASensorNumber::PrecisionP0);
HASensorNumber tempSensor("temperature", HASensorNumber::PrecisionP2);
HASensorNumber humSensor("humididy", HASensorNumber::PrecisionP2);
HASensorNumber wifiLostCount("wifiLostCount", HASensorNumber::PrecisionP0);
HASensorNumber wifiRssi("wifiRssi", HASensorNumber::PrecisionP0);
HASensorNumber currentTempHA("currentTempHA", HASensorNumber::PrecisionP1);
HASensorNumber ADCInput("ADCInput", HASensorNumber::PrecisionP0);  // for diagnostics
HABinarySensor heaterOnHA("heater_onr");
// HASensorNumber targetTempHA("target_temp", HASensorNumber::PrecisionP1);
HANumber targetTempHA("targetBlanket", HANumber::PrecisionP1);

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

void setup() {
  Serial.begin(115200);
  while(!Serial) {
    delay(100);
  }

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
  init_ha(client, device, mqtt, co2Sensor, tempSensor, humSensor);
  currentTempHA.setIcon("mdi:thermometer");
  currentTempHA.setName("blanket current");
  currentTempHA.setUnitOfMeasurement("°C");

  targetTempHA.setIcon("mdi:heating-coil");
  targetTempHA.setName("blanket target");
  targetTempHA.setUnitOfMeasurement("°C");
  targetTempHA.setDeviceClass("temperature");
  targetTempHA.setMin(15.0f);  // not variables
  targetTempHA.setMax(60.0f);
  targetTempHA.setMode(HANumber::ModeSlider);
  targetTempHA.setRetain(true);
  targetTempHA.onCommand(onNumberCommand);

  ADCInput.setName("ADCInput");
  ADCInput.setUnitOfMeasurement("n");

  wifiLostCount.setIcon("mdi:gauge");
  wifiLostCount.setName("WIFI lost count");
  wifiLostCount.setUnitOfMeasurement("n");

  wifiRssi.setIcon("mdi:wifi");
  wifiRssi.setName("WIFI RSSI");
  wifiRssi.setUnitOfMeasurement("dBm");

  heaterOnHA.setIcon("mdi:toggle-switch");
  heaterOnHA.setName("heater blanket");

  // Ініціалізація OTA з паролем
  setupOTA(HOSTNAME, OTA_PASSWORD);
}

unsigned long lastUpdateAt = 0;
const unsigned int wifi_fail_triger = 300000;  // при кількості спроб реконнест

unsigned long lastThermoUpdate = 0;
const unsigned long THERMO_INTERVAL = 2000;  // 2 секунди для термостату

void loop() {
  // ----------------- local -----------------------
  // КНОПКИ - викликаємо в кожному циклі для швидкого відгуку
  handleButtons();
  handle_oled(co2, temperature, humidity);

  // TODO: скидувати таймер при нажиманні кнопок
  if((millis() - lastUpdateAt) > 1000) {  // 1000ms debounce time
    lastUpdateAt = millis();

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
      setupWiFi();
    }
    return;  // не підключені до WiFi
  }

  // you can reset the sensor as follows:
  // analogSensor.setValue(nullptr);

  mqtt.loop();
  ArduinoOTA.handle();
  if(isDataReady) {
    co2Sensor.setValue(co2);
    tempSensor.setValue(temperature);
    humSensor.setValue(humidity);
    currentTempHA.setValue(TempCurrent);
    ADCInput.setValue(analogRead(0));  // для моніторингу
  }

  wifiRssi.setValue(WiFi.RSSI());
  // Можеш публікувати стан термостату в MQTT
  heaterOnHA.setState(relayState);
  // targetTempHA.setValue(TempTarget);
  targetTempHA.setState(TempTarget);
}
