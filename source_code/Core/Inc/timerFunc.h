/*
 * timerFunc.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

#ifndef INC_TIMERFUNC_H_
#define INC_TIMERFUNC_H_

extern volatile uint32_t g_tim2Ticks;  /* 100µs counter, priority 2, used for debouncing */
void timerInit(void (*pcb)(void));
#endif /* INC_TIMERFUNC_H_ */
