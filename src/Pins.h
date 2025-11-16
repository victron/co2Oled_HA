#ifndef PINS_H
#define PINS_H

#define LED 2
#define THERMISTOR_PIN 0  // Пін, до якого підключено термістор (аналоговий пін A0)
#define RELAY_PIN 5
#define BUTTON_DOWN 4
// #define BUTTON_UP 13
#define BUTTON_UP 3
#define ZERO_DETECT 13
#define OLED_SDA D5    // Stock firmware shows wrong pins
#define OLED_SCL D6    // They swap SDA with SCL ;)
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
// #define ZERO_DETECT 3

#endif