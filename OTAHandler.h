#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>

void setupOTA(const char *hostname, const char *password);

#endif // OTA_HANDLER_H
