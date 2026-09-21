#include "app.h"
#include "main.h"

#include <stdint.h>

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;

#define POT_MIN_ANGLE_DEG       0
#define POT_MAX_ANGLE_DEG       270

#define ADC_MAX_VALUE           4095

#define ADC_SAMPLES             16


#define SERVO_MIN_ANGLE_DEG     0
#define SERVO_MAX_ANGLE_DEG     180

#define SERVO_MIN_PULSE_US      500
#define SERVO_MAX_PULSE_US      2400

#define CONTROL_PERIOD_MS       20


static int clamp_int(int value, int min, int max)
{
    if (value < min)
    {
        return min;
    }

    if (value > max)
    {
        return max;
    }

    return value;
}


static uint32_t potentiometer_read(void)
{
    uint32_t sum = 0;

    for (int i = 0; i < ADC_SAMPLES; i++)
    {
        HAL_ADC_Start(&hadc1);

        HAL_ADC_PollForConversion(
            &hadc1,
            10
        );

        uint32_t raw_value =
            HAL_ADC_GetValue(&hadc1);

        sum += raw_value;

        HAL_ADC_Stop(&hadc1);
    }

    return sum / ADC_SAMPLES;
}


static int adc_to_pot_angle(uint32_t adc_value)
{
    adc_value = clamp_int(
        adc_value,
        0,
        ADC_MAX_VALUE
    );

    return (
        adc_value * POT_MAX_ANGLE_DEG
    ) / ADC_MAX_VALUE;
}

static int pot_to_servo_angle(int pot_angle)
{
    if (pot_angle <= SERVO_MIN_ANGLE_DEG)
    {
        return SERVO_MIN_ANGLE_DEG;
    }

    if (pot_angle >= SERVO_MAX_ANGLE_DEG)
    {
        return SERVO_MAX_ANGLE_DEG;
    }

    return pot_angle;
}

static void servo_set_angle(int angle)
{
    angle = clamp_int(
        angle,
        SERVO_MIN_ANGLE_DEG,
        SERVO_MAX_ANGLE_DEG
    );

    uint32_t pulse_us =
        SERVO_MIN_PULSE_US +
        (
            angle *
            (
                SERVO_MAX_PULSE_US -
                SERVO_MIN_PULSE_US
            )
        ) /
        SERVO_MAX_ANGLE_DEG;

    __HAL_TIM_SET_COMPARE(
        &htim1,
        TIM_CHANNEL_1,
        pulse_us
    );
}

void app_init(void)
{

    HAL_TIM_PWM_Start(
        &htim1,
        TIM_CHANNEL_1
    );

    servo_set_angle(0);
}


void app_run(void)
{

    uint32_t adc_value =
        potentiometer_read();


    int pot_angle =
        adc_to_pot_angle(adc_value);


    int servo_angle =
        pot_to_servo_angle(pot_angle);

    servo_set_angle(servo_angle);

    HAL_Delay(CONTROL_PERIOD_MS);
}
