#include "app.h"
#include "i2c.h"         // Згенеровано CubeMX (містить hi2c1)
#include "usbd_cdc_if.h" // Згенеровано CubeMX (містить CDC_Transmit_FS)
#include <stdio.h>
#include <string.h>

// 7-бітна адреса DS1307 = 0x68, для HAL робимо зсув вліво (0xD0)
#define DS1307_I2C_ADDR (0x68 << 1)

// Допоміжні функції перетворення BCD <-> Decimal
static uint8_t bcdToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static uint8_t decToBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

// Зчитування дати та часу з DS1307
HAL_StatusTypeDef DS1307_GetTime(RTC_DateTime *dt) {
    uint8_t get_buf[5];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, DS1307_I2C_ADDR, 0x00,
                                               I2C_MEMADD_SIZE_8BIT,
    get_buf, 7, 100);

    if (status == HAL_OK) {
        dt->seconds     = bcdToDec(get_buf[0] & 0x7F); // Маскуємо біт CH
        dt->minutes     = bcdToDec(get_buf[1]);
        dt->hours       = bcdToDec(get_buf[2] & 0x3F); // 24-годинний формат
        dt->day_of_week = bcdToDec(get_buf[3]);
        dt->day         = bcdToDec(get_buf[4]);
        dt->month       = bcdToDec(get_buf[5]);
        dt->year        = bcdToDec(get_buf[6]);
    }
    return status;
}

// Запис дати та часу в DS1307
HAL_StatusTypeDef DS1307_SetTime(RTC_DateTime *dt) {
    uint8_t set_buf[7];

    set_buf[0] = decToBcd(dt->seconds) & 0x7F; // CH = 0 (запуск осцилятора)
    set_buf[1] = decToBcd(dt->minutes);
    set_buf[2] = decToBcd(dt->hours);
    set_buf[3] = decToBcd(dt->day_of_week);
    set_buf[4] = decToBcd(dt->day);
    set_buf[5] = decToBcd(dt->month);
    set_buf[6] = decToBcd(dt->year);

    return HAL_I2C_Mem_Write(&hi2c1, DS1307_I2C_ADDR, 0x00,
                             I2C_MEMADD_SIZE_8BIT, set_buf, 7, 100);
}

// Перевірка прапорця CH (Clock Halt) та запуск генератора при першому увімкненні
static void DS1307_InitCheck(void) {
    uint8_t reg0 = 0;
    if (HAL_I2C_Mem_Read(&hi2c1, DS1307_I2C_ADDR, 0x00,
I2C_MEMADD_SIZE_8BIT, &reg0, 1, 100) == HAL_OK) {
        // Якщо біт CH (Bit 7) = 1, годинник зупинено — ініціалізуємо базовий час
        if (reg0 & 0x80) {
            RTC_DateTime init_time = {
                .seconds = 0,
                .minutes = 0,
                .hours = 12,
                .day_of_week = 1,
                .day = 1,
                .month = 1,
                .year = 26 // 2026 рік
            };
            DS1307_SetTime(&init_time);
        }
    }
}

// Безпечна відправка рядка через USB CDC з очікуванням готовності буфера
static void USB_Log_Print(const char *str) {
    uint16_t len = (uint16_t)strlen(str);
    uint32_t timeout = 10000;
    while (CDC_Transmit_FS((uint8_t*)str, len) == USBD_BUSY && timeout--) {
        // Очікування звільнення USB-буфера
    }
}

// Початкова ініціалізація додатка
void setup(void) {
    HAL_Delay(1000); // Пауза для стабілізації живлення та USB

    // Перевірка відповіді від DS1307 (3 спроби з тайм-аутом 100 мс)
    if (HAL_I2C_IsDeviceReady(&hi2c1, DS1307_I2C_ADDR, 3, 100) == HAL_OK) {
        USB_Log_Print("-> DS1307 found on I2C bus!\r\n");
        DS1307_InitCheck(); // Запуск осцилятора, якщо зупинений
    } else {
        USB_Log_Print("-> ERROR: DS1307 NOT FOUND on I2C bus!\r\n");
        USB_Log_Print("   Check: 1) VCC connected to 5V; 2) PB6=SCL, PB7=SDA; 3) Wiring contacts.\r\n");
    }

    USB_Log_Print("=== STM32F401 RTC DS1307 USB CDC Logger ===\r\n");
}

// Основний виклик у нескінченному циклі
void loop(void) {
    RTC_DateTime dt;
    char log_buffer[64];

    if (DS1307_GetTime(&dt) == HAL_OK) {
        snprintf(log_buffer, sizeof(log_buffer),
                "RTC Log: %02d.%02d.20%02d | %02d:%02d:%02d\r\n",
                dt.day, dt.month, dt.year,
                dt.hours, dt.minutes, dt.seconds);
        USB_Log_Print(log_buffer);
    } else {
        USB_Log_Print("Error: Failed to read DS1307 via I2C!\r\n");
    }

    HAL_Delay(1000); // Періодичність виведення — 1 секунда
}
