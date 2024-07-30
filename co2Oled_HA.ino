#include <Arduino.h>
#include <ArduinoHA.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

#include "OTAHandler.h"
#include "button.h"
#include "co2sensor.h"
#include "ha_functions.h"
#include "oled.h"

#define HOSTNAME "co2"
#define BUTTON_PIN 13
#define LED 2
bool connected = false;
// globals for sensor
uint16_t co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;
bool isDataReady = false;

// globals for HA
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);
HASensorNumber co2Sensor("co2", HASensorNumber::PrecisionP0);
HASensorNumber tempSensor("temperature", HASensorNumber::PrecisionP2);
HASensorNumber humSensor("humididy", HASensorNumber::PrecisionP2);
HASensorNumber wifiLostCount("wifiLostCount", HASensorNumber::PrecisionP0);
HASensorNumber wifiRssi("wifiRssi", HASensorNumber::PrecisionP0);

HAButton buttonA("button1");
button btn(BUTTON_PIN);

void onButtonCommand(HAButton* sender) {
  if(sender == &buttonA) {
    // для моніторингу
  }
}

bool button_pushed = false;
const char* bath_Led = "?";
const char* bath_state_topic = "aha/bath_fan/fan_switch/stat_t";
bool bathRelay = false;
const char* bath_avty_topic = "aha/bath_fan/avty_t";
bool bathAvty = false;
const char* toilet_Led = "?";
const char* toilet_state_topic = "aha/toilet_fan/fan_switch_toilet/stat_t";
bool toiletRelay = false;
const char* toilet_avty_topic = "aha/toilet_fan/avty_t";
bool toiletAvty = false;

void buttonAction() {
  if(!bathAvty && !toiletAvty) {
    bath_Led = "?";
    toilet_Led = "?";
    button_pushed = false;
    return;
  }
  if(!bathAvty) {
    bath_Led = "?";
    toilet_Led = "^";
    mqtt.publish("aha/toilet_fan/fan_switch_toilet/cmd_t", (toiletRelay ? "OFF" : "ON"));
    return;
  }
  if(!toiletAvty) {
    toilet_Led = "?";
    bath_Led = "^";
    mqtt.publish("aha/bath_fan/fan_switch/cmd_t", (bathRelay ? "OFF" : "ON"));
    return;
  }
  // && щоб вимикати якщо один вкл інший викл. (all available)
  bool allRelay = bathRelay && toiletRelay;
  bath_Led = "^";
  toilet_Led = "^";
  mqtt.publish("aha/toilet_fan/fan_switch_toilet/cmd_t", (allRelay ? "OFF" : "ON"));
  mqtt.publish("aha/bath_fan/fan_switch/cmd_t", (allRelay ? "OFF" : "ON"));
}

bool parseState(const char* message) {
  if(strcmp(message, "ON") == 0) {
    return true;
  } else if(strcmp(message, "OFF") == 0) {
    return false;
  }
  return false;
}

bool parseAvty(const char* message) {
  if(strcmp(message, "online") == 0) {
    return true;
  } else if(strcmp(message, "offline") == 0) {
    return false;
  }
  return false;
}

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

  if(strcmp(topic, bath_state_topic) == 0) {
    bathRelay = parseState(message);
    bath_Led = (bathRelay) ? "B" : ".";
    button_pushed = false;
    return;
  }
  if(strcmp(topic, toilet_state_topic) == 0) {
    toiletRelay = parseState(message);
    toilet_Led = (toiletRelay) ? "T" : ".";
    button_pushed = false;
    return;
  }

  if(strcmp(topic, bath_avty_topic) == 0) {
    bathAvty = parseAvty(message);
    bath_Led = (bathAvty) ? bath_Led : "?";
    return;
  }
  if(strcmp(topic, toilet_avty_topic) == 0) {
    toiletAvty = parseAvty(message);
    toilet_Led = (toiletAvty) ? toilet_Led : "?";
    return;
  }
}

void onMqttConnected() {
  // Please note that you need to subscribe topic each time the connection with the broker is acquired.
  // in this reason all below callbacks needed
  // mqtt.onMessage(onMqttMessage);
  // mqtt.onConnected(onMqttConnected);
  // mqtt.onDisconnected(onMqttDisconnected);
  // mqtt.onStateChanged(onMqttStateChanged);
  Serial.println("Connected to the broker! subscribing");

  // You can subscribe to custom topic if you need
  mqtt.subscribe(bath_state_topic);
  mqtt.subscribe(bath_avty_topic);
  mqtt.subscribe(toilet_state_topic);
  mqtt.subscribe(toilet_avty_topic);
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

  setupWiFi();

  init_oled();
  init_sensor();
  buttonA.onCommand(onButtonCommand);
  mqtt.onMessage(onMqttMessage);
  mqtt.onConnected(onMqttConnected);
  mqtt.onDisconnected(onMqttDisconnected);
  mqtt.onStateChanged(onMqttStateChanged);
  init_ha(client, device, mqtt, co2Sensor, tempSensor, humSensor);

  wifiLostCount.setIcon("mdi:gauge");
  wifiLostCount.setName("WIFI lost count");
  wifiLostCount.setUnitOfMeasurement("n");

  wifiRssi.setIcon("mdi:wifi");
  wifiRssi.setName("WIFI RSSI");
  wifiRssi.setUnitOfMeasurement("dBm");

  buttonA.setIcon("mdi:fan-alert");
  buttonA.setName("Click stat");

  // Serial.println("Subscribing to topic...");
  // if(mqtt.subscribe(bath_state_topic)) {
  //   Serial.println("Subscribed to topic");
  // } else {
  //   Serial.println("Failed to subscribe to topic");
  //   Serial.print("MQTT subscription state: ");
  //   Serial.println(mqtt.getState());
  // }
  // press callbacks
  // Ініціалізація OTA з паролем
  setupOTA(HOSTNAME, OTA_PASSWORD);
}

unsigned long lastUpdateAt = 0;
unsigned int wifi_fail_counter = 0;
const unsigned int wifi_fail_triger = 300000;  // при кількості спроб реконнест
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

  // Перевірка натискання кнопки
  if(btn.click()) {
    button_pushed = true;
    buttonAction();
  }

  mqtt.loop();
  ArduinoOTA.handle();

  if((millis() - lastUpdateAt) > 1000) {  // 1000ms debounce time
    // Read Measurement
    readMeasurement(co2, temperature, humidity, isDataReady);
    handle_oled(co2, temperature, humidity);

    if(isDataReady) {
      co2Sensor.setValue(co2);
      tempSensor.setValue(temperature);
      humSensor.setValue(humidity);
    }

    lastUpdateAt = millis();
    // you can reset the sensor as follows:
    // analogSensor.setValue(nullptr);
    wifiLostCount.setValue(wifi_fail_counter);

    int8_t rssi = WiFi.RSSI();
    wifiRssi.setValue(rssi);
  }
}
