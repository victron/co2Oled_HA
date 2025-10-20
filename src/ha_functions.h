// #ifndef HA_FUNCTIONS_H
// #define HA_FUNCTIONS_H
#include <ArduinoHA.h>
#include <ESP8266WiFi.h>

#include "secrets.h"

void init_ha(WiFiClient& client, HADevice& device, HAMqtt& mqtt, HASensorNumber& co2Sensor, HASensorNumber& tempSensor, HASensorNumber& humSensor);

// #endif