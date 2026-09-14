#include "app.h"
#include "main.h"
#include <stdio.h>

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;

typedef enum
{
    MODE_LED,
    MODE_MOTOR
} ControlMode;

static ControlMode currentMode = MODE_LED;

#define ADC_MAX  4095U
#define PWM_MAX  999U

static uint32_t Read_Potentiometer(void)
{
    HAL_ADC_Start(&hadc1);

    HAL_ADC_PollForConversion(&hadc1, 10);

    uint32_t value = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    return value;
}

static uint32_t ADC_To_PWM(uint32_t adcValue)
{
    return (adcValue * PWM_MAX) / ADC_MAX;
}

static void Set_LED_PWM(uint32_t duty)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}

static void Set_Motor_PWM(uint32_t duty)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
}

void App_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

    Set_LED_PWM(0);
    Set_Motor_PWM(0);

    printf("Application started\r\n");
    printf("MODE: LED\r\n");
}

void App_Run(void)
{
    static GPIO_PinState previousButtonState = GPIO_PIN_RESET;

    GPIO_PinState buttonState =
        HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);

    uint32_t adcValue = Read_Potentiometer();

    uint32_t pwmValue = ADC_To_PWM(adcValue);

    /*
     * Detect button press
     */
    if (buttonState == GPIO_PIN_SET &&
        previousButtonState == GPIO_PIN_RESET)
    {
        if (currentMode == MODE_LED)
        {
            currentMode = MODE_MOTOR;

            Set_LED_PWM(0);

            printf("MODE: MOTOR\r\n");
        }
        else
        {
            currentMode = MODE_LED;

            Set_Motor_PWM(0);

            printf("MODE: LED\r\n");
        }

        HAL_Delay(50);
    }

    previousButtonState = buttonState;


    /*
     * Control selected device
     */

    if (currentMode == MODE_LED)
    {
        Set_LED_PWM(pwmValue);
        Set_Motor_PWM(0);
    }
    else
    {
        Set_Motor_PWM(pwmValue);
        Set_LED_PWM(0);
    }


    /*
     * Debug
     */

    printf(
        "ADC: %lu | PWM: %lu | MODE: %s | BUTTON: %d\r\n",
        adcValue,
        pwmValue,
        currentMode == MODE_LED ? "LED" : "MOTOR",
        buttonState
    );

    HAL_Delay(100);
}
