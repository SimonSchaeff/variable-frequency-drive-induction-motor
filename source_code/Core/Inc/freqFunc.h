/*
 * freqFunc.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-05-09
 *  Author:     Simon Schäffler
 */

#ifndef INC_FREQFUNC_H_
#define INC_FREQFUNC_H_

#include <stdint.h>

/* ============ Exported Variables ============ */

extern volatile uint8_t  freq_pref_bt_cur;
extern volatile uint8_t  freq_pref_poti_cur;
extern volatile uint16_t freq_pref_user;
extern volatile uint16_t freq_motor;

/* ============ Exported Functions ============ */

uint16_t freqSelect(void);
void freqSyncOld(void);
void freqRamp(uint16_t freq);

#endif /* INC_FREQFUNC_H_ */