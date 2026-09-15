#include "app.h"
#include "main.h"

#include <stdbool.h>
#include <stdint.h>

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
// ============================================================
// AUDIO
// ============================================================

#define TICK_MS        50U
#define BUZZER_DUTY   250U


typedef struct
{
    uint32_t frequency;
    uint32_t duration_ms;

} Note;


// ============================================================
// MELODY
// ============================================================

static const Note melody[] =
{
    {262, 300},     // C
    {262, 300},     // C
    {294, 600},     // D

    {262, 300},     // C
    {262, 300},     // C
    {294, 600},     // D

    {262, 300},     // C
    {262, 300},     // C
    {294, 300},     // D
    {330, 800},     // E

    {0,   500}      // Pause
};


#define MELODY_LENGTH \
    (sizeof(melody) / sizeof(melody[0]))


// ============================================================
// STATE
// ============================================================

static volatile bool tick_50ms = false;

static uint32_t current_note = 0;
static uint32_t elapsed_ms = 0;


// ============================================================
// BUZZER
// ============================================================

static void buzzer_start(uint32_t frequency)
{
    if (frequency == 0)
    {
        return;
    }

    /*
     * TIM3 timer clock = 84 MHz
     *
     * Prescaler = 83
     *
     * 84 MHz / (83 + 1) = 1 MHz
     *
     * Therefore:
     *
     * 1 timer tick = 1 us
     *
     * ARR determines the PWM frequency.
     */

    uint32_t period = (1000000UL / frequency) - 1UL;

    __HAL_TIM_SET_AUTORELOAD(&htim3, period);

    /*
     * ~25% duty cycle
     */

    uint32_t pulse = (period + 1UL) / 4UL;

    __HAL_TIM_SET_COMPARE(
        &htim3,
        TIM_CHANNEL_1,
        pulse
    );

    /*
     * Reset counter so the new frequency
     * starts immediately.
     */

    __HAL_TIM_SET_COUNTER(&htim3, 0);

    HAL_TIM_PWM_Start(
        &htim3,
        TIM_CHANNEL_1
    );
}


static void buzzer_stop(void)
{
    HAL_TIM_PWM_Stop(
        &htim3,
        TIM_CHANNEL_1
    );
}


// ============================================================
// PLAY CURRENT NOTE
// ============================================================

static void play_current_note(void)
{
    uint32_t frequency =
        melody[current_note].frequency;

    if (frequency == 0)
    {
        buzzer_stop();
    }
    else
    {
        buzzer_start(frequency);
    }
}


// ============================================================
// UPDATE MELODY
// ============================================================

static void buzzer_update(void)
{
    elapsed_ms += TICK_MS;

    if (elapsed_ms >= melody[current_note].duration_ms)
    {
        elapsed_ms = 0;

        current_note++;

        if (current_note >= MELODY_LENGTH)
        {
            current_note = 0;
        }

        play_current_note();
    }
}


// ============================================================
// INIT
// ============================================================

void app_init(void)
{
    current_note = 0;
    elapsed_ms = 0;

    tick_50ms = false;

    /*
     * Start TIM2 in interrupt mode.
     */

    HAL_TIM_Base_Start_IT(&htim2);

    /*
     * Start first note.
     */

    play_current_note();
}


// ============================================================
// MAIN PROCESS
// ============================================================

void app_process(void)
{
    if (tick_50ms)
    {
        tick_50ms = false;

        buzzer_update();
    }
}


// ============================================================
// TIMER CALLBACK
// ============================================================

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        tick_50ms = true;
    }
}
