
#include "bme280_port.h"

#include "main.h"
#include "spi.h"
#include "bme280.h"
#include "bme280_defs.h"

#include <stddef.h>
#include <string.h>

/* BME280 CS: PA4 */
#define BME280_CS_GPIO_Port GPIOA
#define BME280_CS_Pin       GPIO_PIN_4

static struct bme280_dev bme280;
static struct bme280_settings bme280_settings;
static uint8_t bme280_ready = 0;


/* ---------------- SPI chip select ---------------- */

static void BME280_CS_Select(void)
{
    HAL_GPIO_WritePin(
        BME280_CS_GPIO_Port,
        BME280_CS_Pin,
        GPIO_PIN_RESET
    );
}

static void BME280_CS_Deselect(void)
{
    HAL_GPIO_WritePin(
        BME280_CS_GPIO_Port,
        BME280_CS_Pin,
        GPIO_PIN_SET
    );
}


/* ---------------- Bosch SPI read callback ---------------- */

static BME280_INTF_RET_TYPE BME280_SPI_Read(
    uint8_t reg_addr,
    uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    (void)intf_ptr;

    if ((reg_data == NULL) || (len == 0U))
    {
        return -1;
    }

    /* SPI read: bit 7 = 1 */
    uint8_t address = reg_addr | 0x80U;

    BME280_CS_Select();

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(&hspi1, &address, 1U, 100U);

    if (status == HAL_OK)
    {
        status = HAL_SPI_Receive(
            &hspi1,
            reg_data,
            len,
            100U
        );
    }

    BME280_CS_Deselect();

    return (status == HAL_OK) ? 0 : -1;
}


/* ---------------- Bosch SPI write callback ---------------- */

static BME280_INTF_RET_TYPE BME280_SPI_Write(
    uint8_t reg_addr,
    const uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    (void)intf_ptr;

    if ((reg_data == NULL) || (len == 0U))
    {
        return -1;
    }

    /* SPI write: bit 7 = 0 */
    uint8_t address = reg_addr & 0x7FU;

    BME280_CS_Select();

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(&hspi1, &address, 1U, 100U);

    if (status == HAL_OK)
    {
        status = HAL_SPI_Transmit(
            &hspi1,
            (uint8_t *)reg_data,
            len,
            100U
        );
    }

    BME280_CS_Deselect();

    return (status == HAL_OK) ? 0 : -1;
}


/* ---------------- Bosch delay callback ---------------- */

static void BME280_DelayUs(
    uint32_t period,
    void *intf_ptr)
{
    (void)intf_ptr;

    /* HAL_Delay has millisecond resolution */
    HAL_Delay((period + 999U) / 1000U);
}


/* ---------------- Public initialization ---------------- */

int BME280_Port_Init(void)
{
    bme280_ready = 0;

    BME280_CS_Deselect();

    memset(&bme280, 0, sizeof(bme280));
    memset(&bme280_settings, 0, sizeof(bme280_settings));

    bme280.intf = BME280_SPI_INTF;
    bme280.intf_ptr = NULL;
    bme280.read = BME280_SPI_Read;
    bme280.write = BME280_SPI_Write;
    bme280.delay_us = BME280_DelayUs;

    /* Read chip ID and calibration coefficients */
    int8_t result = bme280_init(&bme280);

    if (result != BME280_OK)
    {
        return (int)result;
    }

    /* Measurement settings */
    bme280_settings.osr_t = BME280_OVERSAMPLING_1X;
    bme280_settings.osr_p = BME280_OVERSAMPLING_4X;
    bme280_settings.osr_h = BME280_OVERSAMPLING_1X;

    bme280_settings.filter = BME280_FILTER_COEFF_4;
    bme280_settings.standby_time = BME280_STANDBY_TIME_0_5_MS;

    result = bme280_set_sensor_settings(
        BME280_SEL_ALL_SETTINGS,
        &bme280_settings,
        &bme280
    );

    if (result != BME280_OK)
    {
        return (int)result;
    }

    result = bme280_set_sensor_mode(
        BME280_POWERMODE_NORMAL,
        &bme280
    );

    if (result != BME280_OK)
    {
        return (int)result;
    }

    bme280_ready = 1;

    return 0;
}


/* ---------------- Public measurement read ---------------- */


int BME280_Port_Read(BME280_PortData *data)
{
    if ((data == NULL) || (bme280_ready == 0U))
    {
        return -1;
    }

    /*
     * Check whether the physical sensor still responds.
     * BME280 chip ID register: 0xD0
     * Expected chip ID: 0x60
     */
    uint8_t chip_id = 0;

    BME280_INTF_RET_TYPE spi_result =
        BME280_SPI_Read(0xD0U, &chip_id, 1U, NULL);

    if ((spi_result != 0) || (chip_id != 0x60U))
    {
        return -2;
    }

    struct bme280_data sensor_data;

    int8_t result = bme280_get_sensor_data(
        BME280_ALL,
        &sensor_data,
        &bme280
    );

    if (result != BME280_OK)
    {
        return (int)result;
    }

    /* Reject obviously invalid measurements */
    if ((sensor_data.temperature < -40.0) ||
        (sensor_data.temperature > 85.0) ||
        (sensor_data.pressure < 30000.0) ||
        (sensor_data.pressure > 110000.0) ||
        (sensor_data.humidity < 0.0) ||
        (sensor_data.humidity > 100.0))
    {
        return -3;
    }

    data->temperature_c = sensor_data.temperature;
    data->pressure_pa = sensor_data.pressure;
    data->humidity_rh = sensor_data.humidity;

    return 0;
}

