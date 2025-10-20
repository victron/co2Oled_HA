#include "ha_functions.h"
void init_ha(WiFiClient& client, HADevice& device, HAMqtt& mqtt, HASensorNumber& co2Sensor, HASensorNumber& tempSensor, HASensorNumber& humSensor) {
  // Unique ID must be set!
  byte mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));

  // connect to wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);  // waiting for the connection
  }
  Serial.println();
  Serial.println("Connected to the network");

  // set device's details (optional)
  device.setName("Air");
  device.setSoftwareVersion("1.0.0");

  // configure sensor (optional)
  co2Sensor.setIcon("mdi:molecule-co2");
  co2Sensor.setName("co2");
  co2Sensor.setUnitOfMeasurement("pps");
  tempSensor.setIcon("mdi:thermometer");
  tempSensor.setName("thermometer");
  tempSensor.setUnitOfMeasurement("°C");
  humSensor.setIcon("mdi:water-percent");
  humSensor.setName("humidity");
  humSensor.setUnitOfMeasurement("%");

  if(mqtt.begin(BROKER_ADDR, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("Connected to MQTT broker");
  } else {
    Serial.println("Failed to connect to MQTT broker");
  }

  device.enableSharedAvailability();
  device.enableLastWill();
}