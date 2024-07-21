#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoHA.h>
#include <Wire.h>

#include "ha_functions.h"
#include "oled.h"
#include "co2sensor.h"
#include "OTAHandler.h"

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

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }

    // OLED used nonstandard SDA and SCL pins
    Wire.begin(D5, D6);

    init_oled();
    init_sensor();
    init_ha(client, device, mqtt, co2Sensor, tempSensor, humSensor);

    // Ініціалізація OTA з паролем
    setupOTA("bath_fan", OTA_PASSWORD);
}

unsigned long lastUpdateAt = 0;
void loop()
{
    mqtt.loop();
    ArduinoOTA.handle();

    if ((millis() - lastUpdateAt) > 1000)
    { // 1000ms debounce time
        // Read Measurement
        readMeasurement(co2, temperature, humidity, isDataReady);
        handle_oled(co2, temperature, humidity);

        if (isDataReady)
        {
            co2Sensor.setValue(co2);
            tempSensor.setValue(temperature);
            humSensor.setValue(humidity);
        }

        lastUpdateAt = millis();
        // you can reset the sensor as follows:
        // analogSensor.setValue(nullptr);
    }
}
