#include "app.h"
#include "main.h"

#include <stdio.h>
#include <stdint.h>


// ============================================================
// Defines
// ============================================================

#define SMA_SIZE                5

#define LIGHT_ON_THRESHOLD      1700
#define LIGHT_OFF_THRESHOLD     1900

#define MEASUREMENT_PERIOD_MS   100


// ============================================================
// External peripherals
// ============================================================

// hadc1 створюється CubeMX у main.c
extern ADC_HandleTypeDef hadc1;


// ============================================================
// LED state
// ============================================================

typedef enum
{
    LED_OFF,
    LED_ON

} LedState;

static LedState led_state = LED_OFF;


// ============================================================
// SMA / Circular Buffer
// ============================================================

static uint16_t adc_buffer[SMA_SIZE];

static uint8_t buffer_index = 0;
static uint8_t buffer_count = 0;

static uint32_t buffer_sum = 0;


// ============================================================
// Private functions
// ============================================================

static uint16_t sma_add(uint16_t new_value);
static void update_led(uint16_t filtered_value);


// ============================================================
// SETUP
// ============================================================

void setup(void)
{
    // Початковий стан LED
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_5,
        GPIO_PIN_RESET
    );


    // Початковий стан state machine
    led_state = LED_OFF;


    // Обнулення SMA
    buffer_index = 0;
    buffer_count = 0;
    buffer_sum = 0;
}


// ============================================================
// LOOP
// ============================================================

void loop(void)
{
    uint16_t raw_value = 0;
    uint16_t filtered_value = 0;


    // --------------------------------------------------------
    // ADC
    // --------------------------------------------------------

    HAL_ADC_Start(&hadc1);


    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        raw_value = HAL_ADC_GetValue(&hadc1);


        // ----------------------------------------------------
        // SMA filtering
        // ----------------------------------------------------

        filtered_value = sma_add(raw_value);


        // ----------------------------------------------------
        // LED state machine + hysteresis
        // ----------------------------------------------------

        update_led(filtered_value);


        // ----------------------------------------------------
        // Logging
        // ----------------------------------------------------

        printf(
            "RAW: %u | SMA: %u | LED: %s\r\n",
            raw_value,
            filtered_value,
            led_state == LED_ON ? "ON" : "OFF"
        );
    }


    HAL_ADC_Stop(&hadc1);


    // --------------------------------------------------------
    // Measurement period
    // --------------------------------------------------------

    HAL_Delay(MEASUREMENT_PERIOD_MS);
}


// ============================================================
// SMA
// ============================================================

static uint16_t sma_add(uint16_t new_value)
{
    // Якщо buffer повний,
    // прибираємо найстаріше значення
    if (buffer_count == SMA_SIZE)
    {
        buffer_sum -= adc_buffer[buffer_index];
    }
    else
    {
        buffer_count++;
    }


    // Запис нового значення
    adc_buffer[buffer_index] = new_value;


    // Додаємо його до суми
    buffer_sum += new_value;


    // Circular buffer
    buffer_index++;

    if (buffer_index >= SMA_SIZE)
    {
        buffer_index = 0;
    }


    // SMA
    return buffer_sum / buffer_count;
}


// ============================================================
// LED state machine + hysteresis
// ============================================================

static void update_led(uint16_t filtered_value)
{
    switch (led_state)
    {
        case LED_OFF:

            // Темно → вмикаємо LED
            if (filtered_value < LIGHT_ON_THRESHOLD)
            {
                led_state = LED_ON;

                HAL_GPIO_WritePin(
                    GPIOA,
                    GPIO_PIN_5,
                    GPIO_PIN_SET
                );
            }

            break;


        case LED_ON:

            // Світло → вимикаємо LED
            if (filtered_value > LIGHT_OFF_THRESHOLD)
            {
                led_state = LED_OFF;

                HAL_GPIO_WritePin(
                    GPIOA,
                    GPIO_PIN_5,
                    GPIO_PIN_RESET
                );
            }

            break;
    }
}
