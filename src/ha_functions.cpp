#include "ha_functions.h"
void init_ha(WiFiClient& client, HADevice& device, HAMqtt& mqtt, HASensorNumber& co2Sensor, HASensorNumber& tempSensor, HASensorNumber& humSensor) {
  // Unique ID must be set!
  byte mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));

  // configure sensor (optional)
  co2Sensor.setIcon("mdi:molecule-co2");
  co2Sensor.setName("co2");
  co2Sensor.setUnitOfMeasurement("pps");
  co2Sensor.setDeviceClass("carbon_dioxide");
  tempSensor.setIcon("mdi:thermometer");
  tempSensor.setName("thermometer");
  tempSensor.setUnitOfMeasurement("°C");
  tempSensor.setDeviceClass("temperature");
  humSensor.setIcon("mdi:water-percent");
  humSensor.setName("humidity");
  humSensor.setUnitOfMeasurement("%");
  humSensor.setDeviceClass("humidity");

  if(mqtt.begin(BROKER_ADDR, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("Connected to MQTT broker");
  } else {
    Serial.println("Failed to connect to MQTT broker");
  }

  device.enableSharedAvailability();
  device.enableLastWill();
}