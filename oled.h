#ifndef OLED_H
#define OLED_H

#include <Adafruit_SSD1306.h>

extern bool bathRelay;
extern bool bathAvty;
extern bool toiletRelay;
extern bool toiletAvty;
extern bool button_pushed;
extern const char* bath_Led;
extern const char* toilet_Led;
void init_oled();
void handle_oled(uint16_t co2, float temperature, float humidity);

#endif