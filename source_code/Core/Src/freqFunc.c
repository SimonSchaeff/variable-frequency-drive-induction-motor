/*
 * freqFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-05-09
 *  Author:     Simon Schäffler
 */

#include "freqFunc.h"
#include "application.h"
#include "gpioFunc.h"
#include <stdbool.h>

/* ============ Defines ============ */

#define FREQ_RAMP_STEP 1
#define FREQ_START_VALUE 0

/* ============ Variables ============ */

volatile uint8_t  freq_pref_bt_cur   = 0;
volatile uint8_t  freq_pref_poti_cur = 0;
volatile uint8_t  freq_pref_bt_old   = 0;
volatile uint8_t  freq_pref_poti_old = 0;
volatile uint16_t freq_pref_user     = 0;   /* target frequency set by user (BT or poti) */
volatile uint16_t freq_motor         = 0;

/* ============ Functions ============ */

/*
 * freqSelect()
 * Checks if BT or poti frequency has changed.
 * BT: any change is accepted immediately (no hysteresis).
 * Poti: only changes >= 3 Hz are accepted (anti-flicker hysteresis).
 * BT has priority over poti.
 * Returns the new target frequency → stored in freq_pref_user.
 */
/*
 * freqSyncOld()
 * Synchronises _old values to _cur.
 * Call while brake is active so that freqSelect() detects no change
 * when the brake is released → prevents a frequency jump.
 */
void freqSyncOld(void)
{
    freq_pref_bt_old   = freq_pref_bt_cur;
    freq_pref_poti_old = freq_pref_poti_cur;
}

uint16_t freqSelect(void){
	static bool startup   = true;
	bool bt_changed       = false;
	bool poti_changed     = false;

	/* On first call: sync _old values to current state so no spurious
	 * change is detected. Motor stays at 0 Hz until user moves poti or
	 * sends a BT command. */
	if (startup)
	{
		freqSyncOld();
		startup = false;
		return FREQ_START_VALUE;
	}

	/* check if BT value changed (no hysteresis – BT input is explicit) */
	if (freq_pref_bt_cur != freq_pref_bt_old)
	{
		bt_changed = true;
		freq_pref_bt_old = freq_pref_bt_cur;
	}

	/* check if Poti value changed */
	if (freq_pref_poti_cur != freq_pref_poti_old)
	{
		poti_changed = true;
		freq_pref_poti_old = freq_pref_poti_cur;
	}

	/* BT has priority over Poti */
	if (bt_changed)
	{
		return (uint16_t)freq_pref_bt_cur;
	}
	else if (poti_changed)
	{
		return (uint16_t)freq_pref_poti_cur;
	}

	/* nothing changed – keep ramping towards the last user target */
	return freq_pref_user;
}

void freqRamp(uint16_t freq)
{
	if (freq_motor < freq)
	{
		/* ramp up: increase freq_motor by one step towards target */
		freq_motor += FREQ_RAMP_STEP;
		if (freq_motor > freq)
			freq_motor = freq;   /* don't overshoot */
	}
	else if (freq_motor > freq)
	{
		/* ramp down: decrease freq_motor by one step towards target */
		if (freq_motor >= FREQ_RAMP_STEP)
			freq_motor -= FREQ_RAMP_STEP;
		else
			freq_motor = 0;      /* don't underflow (uint) */

		if (freq_motor < freq)
			freq_motor = freq;   /* don't undershoot */
	}
	/* else: freq_motor == freq --> nothing to do */
}