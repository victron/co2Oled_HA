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
HABinarySensor heaterOnHA("heater_on");
// HASensorNumber targetTempHA("target_temp", HASensorNumber::PrecisionP1);
HANumber targetTempHA("target_temp", HANumber::PrecisionP1);

void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length) {
  // This callback is called when message from MQTT broker is received.
  Serial.print("New message on topic: ");
  Serial.println(topic);
  // parse data
  Serial.print("Data: ");
  Serial.println((const char*)payload);
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Message: ");
  Serial.println(message);
}

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

void onMqttStateChanged(HAMqtt::ConnectionState state) {
  Serial.print("MQTT state changed to: ");
  Serial.println(static_cast<int8_t>(state));
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi NOT connected!!!!");
    return;
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
  digitalWrite(RELAY_PIN, RELEY_OFF);

  setupWiFi();

  init_oled();
  init_sensor();

  mqtt.onMessage(onMqttMessage);
  mqtt.onConnected(onMqttConnected);
  mqtt.onDisconnected(onMqttDisconnected);
  mqtt.onStateChanged(onMqttStateChanged);
  init_ha(client, device, mqtt, co2Sensor, tempSensor, humSensor);
  currentTempHA.setIcon("mdi:thermometer");
  currentTempHA.setName("currentTempHA");
  currentTempHA.setUnitOfMeasurement("°C");

  targetTempHA.setIcon("mdi:heating-coil");
  targetTempHA.setName("targetTemp");
  targetTempHA.setUnitOfMeasurement("°C");

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
unsigned int wifi_fail_counter = 0;
const unsigned int wifi_fail_triger = 300000;  // при кількості спроб реконнест

unsigned long lastThermoUpdate = 0;
const unsigned long THERMO_INTERVAL = 2000;  // 2 секунди для термостату

void loop() {
  if(!connected && WiFi.status() == WL_CONNECTED) {
    // not mqtt and connected to wifi
    Serial.println("WiFi OK, mqtt NOK");
    digitalWrite(LED, (millis() / 1000) % 2);
  }
  // Перевірка WiFi з'єднання
  if(WiFi.status() != WL_CONNECTED && wifi_fail_counter > wifi_fail_triger) {
    Serial.println("WiFi lost, trying to reconnect...");
    setupWiFi();
  }
  if(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, LOW);
    wifi_fail_counter++;
    Serial.print("WiFi lost, counter=");
    Serial.println(wifi_fail_counter);
    return;
  }

  mqtt.loop();
  ArduinoOTA.handle();
  // КНОПКИ - викликаємо в кожному циклі для швидкого відгуку
  handleButtons();

  // TODO: скидувати таймер при нажиманні кнопок
  if((millis() - lastUpdateAt) > 1000) {  // 1000ms debounce time
    // Read Measurement
    readMeasurement(co2, temperature, humidity, isDataReady);
    currentTemp = readTemperature();  // call it ones per loop
    if(currentState == SETTING) {
      bool relayState = false;
      handle_oled_setting(currentTemp, targetTemp, relayState);
    } else {
      handle_oled(co2, temperature, humidity, currentTemp);
    }

    if(isDataReady) {
      co2Sensor.setValue(co2);
      tempSensor.setValue(temperature);
      humSensor.setValue(humidity);
      currentTempHA.setValue(currentTemp);
      ADCInput.setValue(analogRead(0));  // для моніторингу
    }

    lastUpdateAt = millis();
    // you can reset the sensor as follows:
    // analogSensor.setValue(nullptr);
    wifiLostCount.setValue(wifi_fail_counter);

    int8_t rssi = WiFi.RSSI();
    wifiRssi.setValue(rssi);
  }

  // ТЕРМОСТАТ - окремий таймер (2 секунди)
  if(millis() - lastThermoUpdate >= THERMO_INTERVAL) {
    lastThermoUpdate = millis();
    updateThermostat(currentTemp);

    // Можеш публікувати стан термостату в MQTT
    heaterOnHA.setState(relayState);
    // targetTempHA.setValue(targetTemp);
    targetTempHA.setState(targetTemp);
  }
}
