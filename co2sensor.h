
#include <SensirionI2CScd4x.h>



void printUint16Hex(uint16_t value);
void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2);
void init_sensor();
void readMeasurement(uint16_t& co2, float& temperature, float& humidity, bool& isDataReady);
