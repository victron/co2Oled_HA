#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoHA.h>
#include <Wire.h>

#include "ha_functions.h"
#include "oled.h"
#include "co2sensor.h"
#include "OTAHandler.h"

#define HOSTNAME "co2"
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

void setupWiFi()
{
    delay(10);
    Serial.println();
    Serial.print("connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.hostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }

    // OLED used nonstandard SDA and SCL pins
    Wire.begin(D5, D6);

    setupWiFi();

    init_oled();
    init_sensor();
    init_ha(client, device, mqtt, co2Sensor, tempSensor, humSensor);

    wifiLostCount.setIcon("mdi:gauge");
    wifiLostCount.setName("WIFI lost count");
    wifiLostCount.setUnitOfMeasurement("n");

    wifiRssi.setIcon("mdi:gauge");
    wifiRssi.setName("WIFI RSSI");
    wifiRssi.setUnitOfMeasurement("dBm");

    // Ініціалізація OTA з паролем
    setupOTA("bath_fan", OTA_PASSWORD);
}

unsigned long lastUpdateAt = 0;
unsigned int wifi_fail_counter = 0;
const unsigned int wifi_fail_triger = 300000; // при кількості спроб реконнест
void loop()
{
    // Перевірка WiFi з'єднання
    if (WiFi.status() != WL_CONNECTED && wifi_fail_counter > wifi_fail_triger)
    {
        Serial.println("WiFi lost, trying to reconnect...");
        setupWiFi();
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        wifi_fail_counter++;
        Serial.print("WiFi lost, counter=");
        Serial.println(wifi_fail_counter);
        return;
    }

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
        wifiLostCount.setValue(wifi_fail_counter);

        int8_t rssi = WiFi.RSSI();
        wifiRssi.setValue(rssi);
    }
}
