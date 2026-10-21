#include "app.h"
#include "usbd_cdc_if.h"
#include "bme280_port.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Recovery configuration */
#define BME280_MAX_READ_ERRORS       3U
#define BME280_RECOVERY_INTERVAL_MS  5000U

/* USB command input */
#define USB_RX_QUEUE_SIZE            128U
#define COMMAND_LINE_SIZE            64U

static uint8_t sensor_available = 0U;
static uint8_t consecutive_errors = 0U;
static uint8_t has_measurement = 0U;
static uint32_t last_recovery_attempt = 0U;

static BME280_PortData last_measurement;

/*
 * Single-producer/single-consumer byte queue:
 * producer = USB CDC receive callback, consumer = loop().
 * One slot is kept empty to distinguish full from empty.
 */
static volatile uint8_t rx_queue[USB_RX_QUEUE_SIZE];
static volatile uint16_t rx_head = 0U;
static volatile uint16_t rx_tail = 0U;
static volatile uint8_t rx_overflow = 0U;

static char command_line[COMMAND_LINE_SIZE];
static uint16_t command_length = 0U;
static uint8_t command_too_long = 0U;
static uint8_t last_was_cr = 0U;


/* ---------------- USB CDC logging ---------------- */

static void USB_Log_Print(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    uint16_t len = (uint16_t)strlen(str);

    /* CDC transmission is asynchronous; the CDC driver copies into its TX buffer. */
    for (uint8_t retry = 0U; retry < 50U; retry++)
    {
        uint8_t result = CDC_Transmit_FS((uint8_t *)str, len);

        if (result == USBD_OK)
        {
            return;
        }

        if (result != USBD_BUSY)
        {
            return;
        }

        HAL_Delay(2U);
    }
}


/* ---------------- Sensor initialization and recovery ---------------- */

static uint8_t BME280_TryInit(void)
{
    int result = BME280_Port_Init();

    if (result == 0)
    {
        sensor_available = 1U;
        consecutive_errors = 0U;

        USB_Log_Print("BME280: initialization successful\r\n");
        return 1U;
    }

    sensor_available = 0U;

    char err[96];
    snprintf(err, sizeof(err),
             "BME280: initialization failed, code=%d\r\n", result);
    USB_Log_Print(err);

    return 0U;
}

static void BME280_HandleReadError(int result)
{
    if (consecutive_errors < BME280_MAX_READ_ERRORS)
    {
        consecutive_errors++;
    }

    /* Report only the first consecutive read error. */
    if (consecutive_errors == 1U)
    {
        char err[96];
        snprintf(err, sizeof(err),
                 "BME280: read failed, code=%d\r\n", result);
        USB_Log_Print(err);
    }

    if (consecutive_errors >= BME280_MAX_READ_ERRORS)
    {
        sensor_available = 0U;
        last_recovery_attempt = HAL_GetTick();
        USB_Log_Print(
            "BME280: repeated errors; recovery scheduled\r\n");
    }
}

static void BME280_TryRecovery(void)
{
    uint32_t now = HAL_GetTick();

    if (sensor_available)
    {
        return;
    }

    if ((uint32_t)(now - last_recovery_attempt) <
        BME280_RECOVERY_INTERVAL_MS)
    {
        return;
    }

    last_recovery_attempt = now;
    USB_Log_Print("BME280: attempting recovery\r\n");

    if (BME280_TryInit())
    {
        USB_Log_Print("BME280: recovery successful\r\n");
    }
}


/* ---------------- Measurement commands ---------------- */

static void Command_Read(void)
{
    if (!sensor_available)
    {
        USB_Log_Print(
            "Error: BME280 is unavailable. Waiting for recovery.\r\n");
        return;
    }

    BME280_PortData data;
    int result = BME280_Port_Read(&data);

    if (result != 0)
    {
        BME280_HandleReadError(result);
        USB_Log_Print("Error: measurement failed.\r\n");
        return;
    }

    consecutive_errors = 0U;
    last_measurement = data;
    has_measurement = 1U;

    USB_Log_Print("Data read successfully.\r\n");
}

static void Command_ShowTemperature(void)
{
    if (!has_measurement)
    {
        USB_Log_Print("No measurement yet. Run 'read' first.\r\n");
        return;
    }

    char output[64];
    snprintf(output, sizeof(output),
             "Temperature: %.2f C\r\n",
             last_measurement.temperature_c);
    USB_Log_Print(output);
}

static void Command_ShowPressure(void)
{
    if (!has_measurement)
    {
        USB_Log_Print("No measurement yet. Run 'read' first.\r\n");
        return;
    }

    char output[96];
    snprintf(output, sizeof(output),
             "Pressure: %.2f hPa (%.1f mmHg)\r\n",
             last_measurement.pressure_pa / 100.0,
             last_measurement.pressure_pa * 0.00750062);
    USB_Log_Print(output);
}

static void Command_ShowHumidity(void)
{
    if (!has_measurement)
    {
        USB_Log_Print("No measurement yet. Run 'read' first.\r\n");
        return;
    }

    char output[64];
    snprintf(output, sizeof(output),
             "Humidity: %.2f %%RH\r\n",
             last_measurement.humidity_rh);
    USB_Log_Print(output);
}

static void Command_ShowHelp(void)
{
    USB_Log_Print(
        "\r\nAvailable commands:\r\n"
        "  help      - show this help\r\n"
        "  read      - read and save all BME280 values\r\n"
        "  temp      - show last saved temperature\r\n"
        "  pressure  - show last saved pressure\r\n"
        "  humidity  - show last saved humidity\r\n"
        "\r\n");
}

static void ProcessCommand(char *command)
{
    /* Trim leading/trailing whitespace and normalize to lowercase. */
    char *start = command;

    while ((*start != '\0') && isspace((unsigned char)*start))
    {
        start++;
    }

    size_t length = strlen(start);
    while ((length > 0U) &&
           isspace((unsigned char)start[length - 1U]))
    {
        start[--length] = '\0';
    }

    for (size_t i = 0U; i < length; i++)
    {
        start[i] = (char)tolower((unsigned char)start[i]);
    }

    if (length == 0U)
    {
        return;
    }

    if (strcmp(start, "help") == 0)
    {
        Command_ShowHelp();
    }
    else if (strcmp(start, "read") == 0)
    {
        Command_Read();
    }
    else if (strcmp(start, "temp") == 0)
    {
        Command_ShowTemperature();
    }
    else if (strcmp(start, "pressure") == 0)
    {
        Command_ShowPressure();
    }
    else if (strcmp(start, "humidity") == 0)
    {
        Command_ShowHumidity();
    }
    else
    {
        USB_Log_Print("Unknown command. Type 'help' for commands.\r\n");
    }
}


/* ---------------- USB receive queue ---------------- */

void App_USB_Receive(const uint8_t *data, uint32_t len)
{
    if (data == NULL)
    {
        return;
    }

    for (uint32_t i = 0U; i < len; i++)
    {
        uint16_t next = (uint16_t)((rx_head + 1U) % USB_RX_QUEUE_SIZE);

        if (next == rx_tail)
        {
            rx_overflow = 1U;
            break;
        }

        rx_queue[rx_head] = data[i];
        rx_head = next;
    }
}

static uint8_t App_USB_GetByte(uint8_t *byte)
{
    if ((byte == NULL) || (rx_tail == rx_head))
    {
        return 0U;
    }

    *byte = rx_queue[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1U) % USB_RX_QUEUE_SIZE);
    return 1U;
}

static void ProcessUSBInput(void)
{
    if (rx_overflow)
    {
        /* Drop pending input and reset the current command after queue overflow. */
        rx_tail = rx_head;
        rx_overflow = 0U;
        command_length = 0U;
        command_too_long = 0U;
        USB_Log_Print("Input overflow. Please enter the command again.\r\n");
    }

    uint8_t byte;

    while (App_USB_GetByte(&byte))
    {
        if ((byte == '\r') || (byte == '\n'))
        {
            /* Terminals may send CRLF; treat it as one line ending. */
            if ((byte == '\n') && last_was_cr)
            {
                last_was_cr = 0U;
                continue;
            }

            last_was_cr = (byte == '\r') ? 1U : 0U;

            if (command_too_long)
            {
                USB_Log_Print("Command too long. Maximum length is 63 characters.\r\n");
            }
            else
            {
                command_line[command_length] = '\0';
                ProcessCommand(command_line);
            }

            command_length = 0U;
            command_too_long = 0U;
        }
        else
        {
            last_was_cr = 0U;

            if (!command_too_long)
            {
                if (command_length < (COMMAND_LINE_SIZE - 1U))
                {
                    command_line[command_length++] = (char)byte;
                }
                else
                {
                    command_too_long = 1U;
                }
            }
        }
    }
}


/* ---------------- Application initialization ---------------- */

void setup(void)
{
    HAL_Delay(1000U);

    sensor_available = 0U;
    consecutive_errors = 0U;
    has_measurement = 0U;
    last_recovery_attempt = HAL_GetTick() - BME280_RECOVERY_INTERVAL_MS;

    USB_Log_Print("\r\nSTM32 BME280 CLI ready.\r\n");
    Command_ShowHelp();

    if (BME280_TryInit())
    {
        USB_Log_Print("Sensor ready. Enter 'read' to take a measurement.\r\n");
    }
}


/* ---------------- Application loop ---------------- */

void loop(void)
{
    ProcessUSBInput();
    BME280_TryRecovery();

    /* No periodic sensor reads or periodic measurement output. */
}
