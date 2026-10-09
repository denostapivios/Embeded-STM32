#ifndef APP_H
#define APP_H

#include "main.h"

// Структура для зберігання дати та часу
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} RTC_DateTime;

// Основні функції проєкту
void setup(void);
void loop(void);

#endif /* APP_H */
