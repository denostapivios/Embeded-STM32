
#include "app.h"
#include "usbd_cdc_if.h"
#include "bme280_port.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Recovery configuration */
#define BME280_MAX_READ_ERRORS       3U
#define BME280_RECOVERY_INTERVAL_MS  5000U
#define BME280_READ_INTERVAL_MS      1000U

static uint8_t sensor_available = 0;
static uint8_t consecutive_errors = 0;
static uint32_t last_recovery_attempt = 0;


/* ---------------- USB CDC logging ---------------- */

static void USB_Log_Print(const char *str)
{
    uint16_t len = (uint16_t)strlen(str);

    for (uint8_t retry = 0; retry < 50U; retry++)
    {
        uint8_t result =
            CDC_Transmit_FS((uint8_t *)str, len);

        if (result == USBD_OK)
        {
            return;
        }

        if (result != USBD_BUSY)
        {
            return;
        }

        HAL_Delay(2);
    }
}


/* ---------------- Sensor recovery ---------------- */

static uint8_t BME280_TryInit(void)
{
    int result = BME280_Port_Init();

    if (result == 0)
    {
        sensor_available = 1;
        consecutive_errors = 0;

        USB_Log_Print(
            "BME280: initialization successful\r\n"
        );

        return 1;
    }

    sensor_available = 0;

    char err[96];

    snprintf(
        err,
        sizeof(err),
        "BME280: initialization failed, code=%d\r\n",
        result
    );

    USB_Log_Print(err);

    return 0;
}


/* ---------------- Application initialization ---------------- */

void setup(void)
{
    HAL_Delay(1000);

    sensor_available = 0;
    consecutive_errors = 0;

    /* Permit an immediate retry if initial setup fails */
    last_recovery_attempt =
        HAL_GetTick() - BME280_RECOVERY_INTERVAL_MS;

    if (BME280_TryInit())
    {
        USB_Log_Print(
            "BME280: monitoring started\r\n"
        );
    }
}


/* ---------------- Application loop ---------------- */

void loop(void)
{
    uint32_t now = HAL_GetTick();

    /* Sensor unavailable: retry initialization periodically */
    if (!sensor_available)
    {
        if ((uint32_t)(now - last_recovery_attempt) >=
            BME280_RECOVERY_INTERVAL_MS)
        {
            last_recovery_attempt = now;

            USB_Log_Print(
                "BME280: attempting recovery\r\n"
            );

            if (BME280_TryInit())
            {
                USB_Log_Print(
                    "BME280: recovery successful\r\n"
                );
            }
        }

        HAL_Delay(BME280_READ_INTERVAL_MS);
        return;
    }

    BME280_PortData data;

    int result = BME280_Port_Read(&data);

    if (result != 0)
    {
        if (consecutive_errors < BME280_MAX_READ_ERRORS)
        {
            consecutive_errors++;
        }

        /* Log the first error, not every failed read */
        if (consecutive_errors == 1U)
        {
            char err[96];

            snprintf(
                err,
                sizeof(err),
                "BME280: read failed, code=%d\r\n",
                result
            );

            USB_Log_Print(err);
        }

        if (consecutive_errors >= BME280_MAX_READ_ERRORS)
        {
            sensor_available = 0;
            last_recovery_attempt = HAL_GetTick();

            USB_Log_Print(
                "BME280: repeated errors; recovery scheduled\r\n"
            );
        }

        HAL_Delay(BME280_READ_INTERVAL_MS);
        return;
    }

    /* A successful read resets the error counter */
    consecutive_errors = 0;

    double pressure_hpa = data.pressure_pa / 100.0;
    double pressure_mmhg = data.pressure_pa * 0.00750062;

    char log_buf[160];

    snprintf(
        log_buf,
        sizeof(log_buf),
        "BME280: Temp=%.2f C | Pressure=%.2f hPa "
        "(%.1f mmHg) | Humidity=%.2f %%RH\r\n",
        data.temperature_c,
        pressure_hpa,
        pressure_mmhg,
        data.humidity_rh
    );

    USB_Log_Print(log_buf);

    HAL_Delay(BME280_READ_INTERVAL_MS);
}
