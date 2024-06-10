// #ifndef HA_FUNCTIONS_H
// #define HA_FUNCTIONS_H
// #include <ESP8266WiFi.h>
#include <ArduinoHA.h>
#include "secrets.h"

void init_ha(WiFiClient &client, HADevice &device, HAMqtt &mqtt, HASensorNumber &co2Sensor, HASensorNumber &tempSensor, HASensorNumber &humSensor);

// #endif