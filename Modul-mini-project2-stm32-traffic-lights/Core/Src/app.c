#include "app.h"
#include "main.h"

#include <stdbool.h>

#define GREEN_TIME_MS          5000
#define GREEN_BLINK_TIME_MS    3000
#define YELLOW_TIME_MS         2000
#define RED_TIME_MS            5000
#define RED_YELLOW_TIME_MS     2000

#define BLINK_INTERVAL_MS      500
#define YELLOW_BLINK_TIME_MS   4000

#define DEBOUNCE_TIME_MS       50

typedef enum
{
    STATE_GREEN,
    STATE_GREEN_BLINK,
    STATE_YELLOW,
    STATE_RED,
    STATE_RED_YELLOW,
    STATE_YELLOW_BLINK

} TrafficLightState;

static volatile bool buttonPressed = false;
static volatile uint32_t lastButtonInterruptTime = 0;

static TrafficLightState currentState = STATE_GREEN;

static uint32_t stateStartTime = 0;
static uint32_t lastBlinkTime = 0;

static bool greenLedState = true;
static bool yellowLedState = false;

static void setLights(bool red, bool yellow, bool green)
{
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_0,
        red ? GPIO_PIN_SET : GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_1,
        yellow ? GPIO_PIN_SET : GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_2,
        green ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
}

void setup(void)
{
    currentState = STATE_GREEN;

    stateStartTime = HAL_GetTick();
    lastBlinkTime = stateStartTime;

    greenLedState = true;
    yellowLedState = false;

    setLights(false, false, true);
}

void loop(void)
{
    uint32_t currentTime = HAL_GetTick();

    if (buttonPressed)
    {
        buttonPressed = false;

        if (currentState != STATE_YELLOW_BLINK)
        {
            currentState = STATE_YELLOW_BLINK;

            stateStartTime = currentTime;
            lastBlinkTime = currentTime;

            yellowLedState = true;

            setLights(false, true, false);
        }
    }

    switch (currentState)
    {
        case STATE_GREEN:

            if (currentTime - stateStartTime >= GREEN_TIME_MS)
            {
                currentState = STATE_GREEN_BLINK;

                stateStartTime = currentTime;
                lastBlinkTime = currentTime;

                greenLedState = true;

                setLights(false, false, true);
            }

            break;

        case STATE_GREEN_BLINK:

            if (currentTime - stateStartTime >= GREEN_BLINK_TIME_MS)
            {
                currentState = STATE_YELLOW;

                stateStartTime = currentTime;

                setLights(false, true, false);

                break;
            }

            if (currentTime - lastBlinkTime >= BLINK_INTERVAL_MS)
            {
                lastBlinkTime = currentTime;

                greenLedState = !greenLedState;

                HAL_GPIO_WritePin(
                    GPIOA,
                    GPIO_PIN_2,
                    greenLedState
                        ? GPIO_PIN_SET
                        : GPIO_PIN_RESET
                );
            }

            break;

        case STATE_YELLOW:

            if (currentTime - stateStartTime >= YELLOW_TIME_MS)
            {
                currentState = STATE_RED;

                stateStartTime = currentTime;

                setLights(true, false, false);
            }

            break;

        case STATE_RED:

            if (currentTime - stateStartTime >= RED_TIME_MS)
            {
                currentState = STATE_RED_YELLOW;

                stateStartTime = currentTime;

                setLights(true, true, false);
            }

            break;

        case STATE_RED_YELLOW:

            if (currentTime - stateStartTime >= RED_YELLOW_TIME_MS)
            {
                currentState = STATE_GREEN;

                stateStartTime = currentTime;

                setLights(false, false, true);
            }

            break;

        case STATE_YELLOW_BLINK:

            if (currentTime - stateStartTime >= YELLOW_BLINK_TIME_MS)
            {
                currentState = STATE_GREEN;

                stateStartTime = currentTime;

                setLights(false, false, true);

                break;
            }

            if (currentTime - lastBlinkTime >= BLINK_INTERVAL_MS)
            {
                lastBlinkTime = currentTime;

                yellowLedState = !yellowLedState;

                HAL_GPIO_WritePin(
                    GPIOA,
                    GPIO_PIN_1,
                    yellowLedState
                        ? GPIO_PIN_SET
                        : GPIO_PIN_RESET
                );
            }

            break;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        uint32_t currentTime = HAL_GetTick();

        if (currentTime - lastButtonInterruptTime >= DEBOUNCE_TIME_MS)
        {
            buttonPressed = true;
            lastButtonInterruptTime = currentTime;
        }
    }
}