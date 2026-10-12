
#ifndef BME280_PORT_H
#define BME280_PORT_H

#include <stdint.h>

typedef struct
{
    double temperature_c;
    double pressure_pa;
    double humidity_rh;
} BME280_PortData;

/**
 * @brief Initialize BME280 via Bosch SensorAPI and STM32 HAL SPI.
 * @return 0 on success, negative value on failure.
 */
int BME280_Port_Init(void);

/**
 * @brief Read compensated temperature, pressure and humidity.
 * @param data Pointer to measurement output.
 * @return 0 on success, negative value on failure.
 */
int BME280_Port_Read(BME280_PortData *data);

#endif /* BME280_PORT_H */
