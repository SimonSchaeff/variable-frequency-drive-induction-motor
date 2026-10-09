/*
 * gpioFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

#include "main.h"
#include "gpioFunc.h"
#include "pwmFunc.h"
#include "timerFunc.h"

/* ============ Variables ============ */

volatile bool brakeActive = false;

/* ============ Functions ============ */

void gpioInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	/* --- Enable clocks --- */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/* --- LED0 (PB5) --- */
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin   = LED0_Pin;
	HAL_GPIO_Init(LED0_GPIO_Port, &GPIO_InitStruct);
	HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);

	/* --- ITRIP (PC5) – digital output, default LOW --- */
	GPIO_InitStruct.Pin   = ITRIP_Pin;
	HAL_GPIO_Init(ITRIP_GPIO_Port, &GPIO_InitStruct);
	HAL_GPIO_WritePin(ITRIP_GPIO_Port, ITRIP_Pin, GPIO_PIN_RESET);

	/* --- RELAIS (PC4) – digital output, default LOW --- */
	GPIO_InitStruct.Pin   = RELAY_PIN;
	HAL_GPIO_Init(RELAY_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_RESET); /* important: relay must be open at startup */

	/* --- BTN1 (PC9, EXTI9_5) --- */
	GPIO_InitStruct.Pin  = BTN1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;   /* btn is low active */
	HAL_GPIO_Init(BTN1_GPIO_Port, &GPIO_InitStruct);
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, BTN1_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/* Button ISR – BTN1 on PC9 */
void EXTI9_5_IRQHandler(void)
{
	__HAL_GPIO_EXTI_CLEAR_FLAG(BTN1_Pin);

	/* Software debouncing: ignore triggers within 200ms of the last one.
	 * g_tim2Ticks runs in TIM2 ISR (priority 2) → independent of SysTick.
	 * 200ms = 2000 ticks at 100µs per tick. */
	static uint32_t s_lastTick = 0;
	if ((g_tim2Ticks - s_lastTick) < 2000U) return;
	s_lastTick = g_tim2Ticks;

	if (!brakeActive)
	{
		brakeActive = true;
		spwmBrake();
	}
	else
	{
		brakeActive = false;
	}
}

void gpioTogglePin(GPIO_TypeDef *port, uint16_t pin)
{
	HAL_GPIO_TogglePin(port, pin);
}

void gpioSetPin(GPIO_TypeDef *port, uint16_t pin, int on)
{
	HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
