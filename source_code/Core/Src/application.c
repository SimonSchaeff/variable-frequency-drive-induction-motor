/*
 * application.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

#include "main.h"
#include "gpioFunc.h"
#include "timerFunc.h"
#include "adFunc.h"
#include "scheduler.h"
#include "pwmFunc.h"
#include "string.h"
#include "sevSegFunc.h"
#include "bluetoothFunc.h"
#include <stdbool.h>
#include "freqFunc.h"
/* ============ Defines ============ */

#define RELAY_STARTUP_PERIOD 6 /* stop value (counter runs 0..5, then freezes) */
#define RELAY_STARTUP_TIME   5 /* at this count (5 × 200ms = 1s) relay closes  */
#define CURRENT_MAX 12         /* [A] maximum phase current                     */

/* ============ Forward Declaration ============ */
/* (freqSelect / freqRamp are declared in freqFunc.h) */

/* ============ Variables ============ */

volatile uint32_t task1Count = 0;
volatile uint32_t task2Count = 0;
volatile uint32_t task3Count = 0;
volatile uint32_t task4Count = 0;

PhaseCurrents_t currents;
VFOInfo_t GateDriverState;

   /* actual frequency, slowly ramped to target  */

void timercallback(void)
{
	schTick();   /* 100µs tick to scheduler */
}

void setup(void)
{
	gpioInit();
	sevSegInit();
	schInit();
	timerInit(timercallback);
	ADInit();
	btInit();
}


/* --------- task1 – every 200ms: ---------
 * toggle LED (cpu is running no failure mode)
 * closing relay at startup
 */
void task1(void)
{
	task1Count++;

	/* locic for closing relay at startup after delay */
	static uint8_t relay_cnt = 0;
	if (relay_cnt < RELAY_STARTUP_PERIOD)
	{
		relay_cnt++;
		if (relay_cnt == RELAY_STARTUP_TIME)
		{
			gpioSetPin(RELAY_PORT, RELAY_PIN, true);
		}
	}
	if (GateDriverState.Failure == false)
	{
		/* toggle LED slowly*/
		gpioTogglePin(LED0_GPIO_Port, LED0_Pin);
	}
}

/* task2 – every 10ms: read ADC */
void task2(void)
{
	task2Count++;

	/* display frequency on seven segment */
	sevSegDecOut(freq_motor);

	/* ramp up or down frequency not instantly */
	freqRamp(freq_pref_user);

	/* read bluetooth input */
	freq_pref_bt_cur = btmain();
}

/* task3 – every 1ms: free for use */
void task3(void)
{
	task3Count++;

	/* read out phase currents */
	getPhaseCurrents(&currents);

	/* read out state of gate driver with VFO pin (temperature and failure) */
	getVfoValue(&GateDriverState);

	if ((currents.I_U > CURRENT_MAX) || (currents.I_V > CURRENT_MAX) || (currents.I_W > CURRENT_MAX) || (GateDriverState.Failure == true))
	{
		/* emergency case */
		gpioSetPin(ITRIP_GPIO_Port, ITRIP_Pin, true); /* irreversible – MCU must be reset after this critical fault */
	}
	
	/* background task to keep seven segment running */
	sevSegTask();

	/* Always read inputs (even during braking) to keep _cur values up to date */
	freq_pref_poti_cur = potiToFreqHz();

	if (!brakeActive)
	{
		/* determine user frequency (poti or bluetooth?) */
		freq_pref_user = freqSelect();
	}
	else
	{
		/* Brake active: keep _old in sync → no frequency jump on release */
		freqSyncOld();
	}
}


/* task4 – every 0.1ms: unused (spwmUpdate called from TIM1_UP_IRQHandler) */
void task4(void)
{
	task4Count++;
}

void loop(void)
{
	schRun();   /* scheduler executes all pending tasks */
}
