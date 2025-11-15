#include "co2sensor.h"

SensirionI2cScd4x scd4x;

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void init_sensor() {
  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire, SCD40_I2C_ADDR_62);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if(error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  uint64_t serialNumber = 0;
  error = scd4x.getSerialNumber(serialNumber);
  if(error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    // Конвертувати uint64_t в три uint16_t для виводу
    uint16_t serial0 = (serialNumber >> 32) & 0xFFFF;
    uint16_t serial1 = (serialNumber >> 16) & 0xFFFF;
    uint16_t serial2 = serialNumber & 0xFFFF;
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if(error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.println("Waiting for first measurement... (5 sec)");
}

void readMeasurement(uint16_t& co2, float& temperature, float& humidity, bool& isDataReady) {
  uint16_t error;
  char errorMessage[256];
  // bool isDataReady = false;
  error = scd4x.getDataReadyStatus(isDataReady);
  if(error) {
    Serial.print("Error trying to execute getDataReadyFlag(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    return;
  }
  if(!isDataReady) {
    return;
  }
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if(error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if(co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
  }
  // else {
  //     Serial.print("Co2:");
  //     Serial.print(co2);
  //     Serial.print("\t");
  //     Serial.print("Temperature:");
  //     Serial.print(temperature);
  //     Serial.print("\t");
  //     Serial.print("Humidity:");
  //     Serial.println(humidity);
  // }
}
