#include "app.h"


// ============================================================
// UART
// ============================================================

extern UART_HandleTypeDef huart1;

static uint8_t rx_data;


// ============================================================
// BUTTON
// ============================================================

static volatile uint8_t button_pressed = 0;

static uint32_t last_button_time = 0;


// ============================================================
// INITIALIZATION
// ============================================================

void APP_Init(void)
{
    // Початковий стан LED
    HAL_GPIO_WritePin(
        LED_GPIO_Port,
        LED_Pin,
        GPIO_PIN_RESET
    );

    // Запускаємо прийом 1 байта через UART interrupt
    HAL_UART_Receive_IT(
        &huart1,
        &rx_data,
        1
    );
}


// ============================================================
// MAIN LOOP
// ============================================================

void APP_Loop(void)
{
    // --------------------------------------------------------
    // STM32 BUTTON
    // --------------------------------------------------------

    if (button_pressed)
    {
        button_pressed = 0;

        // '2' = перемкнути LED на ESP32
        uint8_t command = '2';

        HAL_UART_Transmit(
            &huart1,
            &command,
            1,
            HAL_MAX_DELAY
        );
    }
}


// ============================================================
// GPIO EXTI CALLBACK
// ============================================================

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BUTTON_Pin)
    {
        uint32_t current_time = HAL_GetTick();

        // Debounce 500 ms
        if ((current_time - last_button_time) >= 500)
        {
            button_pressed = 1;
            last_button_time = current_time;
        }
    }
}


// ============================================================
// UART RX CALLBACK
// ============================================================

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // ESP32 відправив '1'
        if (rx_data == '1')
        {
            HAL_GPIO_TogglePin(
                LED_GPIO_Port,
                LED_Pin
            );
        }

        // Знову запускаємо прийом
        HAL_UART_Receive_IT(
            &huart1,
            &rx_data,
            1
        );
    }
}
