/*
 * timerFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

#include "main.h"
#include "pwmFunc.h"

/* ============ Defines ============ */

#define SCHEDULER_PRIO 2        /* priority 0 = highest, 15 = lowest */
#define PWM_GENERATION_PRIO 1   /* priority 0 = highest, 15 = lowest */

/* ============ Variables ============ */

static TIM_HandleTypeDef s_TimerInstance;

volatile uint32_t g_tim2Ticks = 0;  /* incremented every 100µs in TIM2 ISR, priority 2 */

void (*cb)(void);   /* function pointer to the timer callback */

/* ============ Functions ============ */

/* Timer initialisation */
void timerInit(void (*pcb)(void))
{
    /* Timer 2  for scheduling */
	__TIM2_CLK_ENABLE();
	__HAL_DBGMCU_FREEZE_TIM2();
	s_TimerInstance.Instance = TIM2;
    s_TimerInstance.Init.Prescaler = 71;        /* sys clock 72 MHz → timer clock 1 MHz */
    s_TimerInstance.Init.CounterMode = TIM_COUNTERMODE_UP;
    /* timer interrupt every 0.1ms (SET THIS LINE TO CHANGE IT) */
    s_TimerInstance.Init.Period = 99;           /* 1 MHz / 100 = 10 kHz → tick every 100µs */
    s_TimerInstance.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    s_TimerInstance.Init.RepetitionCounter = 0;
    cb = pcb;

    HAL_TIM_Base_Init(&s_TimerInstance);
    HAL_TIM_Base_Start_IT(&s_TimerInstance);    /* set bit 0 in DIER */
    __HAL_TIM_CLEAR_FLAG(&s_TimerInstance, TIM_FLAG_UPDATE);  /* prevent interrupt from initialisation */
    HAL_NVIC_SetPriority(TIM2_IRQn, SCHEDULER_PRIO, 0);     /* Priority 2: Scheduler, may be preempted by TIM1 and button */
    HAL_NVIC_EnableIRQ(TIM2_IRQn);



    /* ---------------------------------------------------------------
     * TIM1: 3-Phase Center-Aligned SPWM with complementary outputs
     *
     *  High-Side:  TIM1_CH1=PA8,  TIM1_CH2=PA9,  TIM1_CH3=PA10
     *  Low-Side:   TIM1_CH1N=PB13, TIM1_CH2N=PB14, TIM1_CH3N=PB15
     *
     *  Carrier:  72 MHz / (2 * 36000) = 1 kHz
     *  Deadtime: ~1µs (hardware, BDTR register)
     *  ISR at peak (URS=1): compute sine → set CCR1/2/3
     *  Hardware comparator: CNT < CCR → CHx HIGH, CHxN LOW
     *                       CNT > CCR → CHx LOW,  CHxN HIGH
     * --------------------------------------------------------------- */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* High-Side Pins: PA8, PA9, PA10 → AF Push-Pull */
    GPIO_InitTypeDef gpio;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pin   = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* Low-Side Pins: PB13, PB14, PB15 → AF Push-Pull */
    gpio.Pin   = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* TIM1 Base: Center-Aligned, 10 kHz Carrier */
    static TIM_HandleTypeDef htim1;
    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 0;
    htim1.Init.CounterMode       = TIM_COUNTERMODE_CENTERALIGNED1; /* 0 → 3600 → 0 */
    htim1.Init.Period            = 7200;   /* 72 MHz / (2*36000) = 1 kHz           */
                                           /* 72 MHz / (2*7200)  = 5 kHz            */
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 1;   /* Center-Aligned generates UEV at both peak AND trough!
                                         * RCR=1: UIF fires only every 2nd UEV = once per
                                         * carrier period = 1 kHz.
                                         * NOTE: URS=1 does NOT suppress the trough UEV! */
    HAL_TIM_PWM_Init(&htim1);

    /* Output Compare: PWM Mode 1 for all 3 channels
     * CNT < CCR → CHx HIGH, CHxN LOW
     * CNT > CCR → CHx LOW,  CHxN HIGH  (+ dead time) */
    TIM_OC_InitTypeDef oc;
    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = 3600;                    /* initial value 50% = Period/2 (sine = 0) */
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;     /* CHx:  normal                    */
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;    /* CHxN: complementary             */
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;   /* CHx  LOW in IDLE state          */
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;  /* CHxN LOW in IDLE state          */
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_2);
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_3);

    /* Break and Deadtime Register (BDTR):
     * Deadtime = DTG * t_DTS, t_DTS = 1/72MHz ≈ 13.9ns
     * DTG = 72 → Deadtime ≈ 72 * 13.9ns ≈ 1µs                            */
    TIM_BreakDeadTimeConfigTypeDef bdtr;
    bdtr.OffStateRunMode  = TIM_OSSR_DISABLE;
    bdtr.OffStateIDLEMode = TIM_OSSI_DISABLE;
    bdtr.LockLevel        = TIM_LOCKLEVEL_OFF;
    bdtr.DeadTime         = 72;               /* ~1µs dead time              */
    bdtr.BreakState       = TIM_BREAK_DISABLE;
    bdtr.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
    bdtr.AutomaticOutput  = TIM_AUTOMATICOUTPUT_ENABLE;
    HAL_TIMEx_ConfigBreakDeadTime(&htim1, &bdtr);

    /* ISR only at peak (URS=1: UIF only on overflow, not on underflow) */
    TIM1->CR1 |= TIM_CR1_URS;
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
    HAL_NVIC_SetPriority(TIM1_UP_IRQn, PWM_GENERATION_PRIO, 0);  /* Priority 1: SPWM, may be preempted by button */
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);

    /* Start all 3 channels and their complementary outputs */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
}

/* ISR of Timer 2 */
void TIM2_IRQHandler(void)
{
	__HAL_TIM_CLEAR_FLAG(&s_TimerInstance, TIM_FLAG_UPDATE);
	g_tim2Ticks++;   /* 100µs per tick → 10000 ticks = 1s */
	cb();
}