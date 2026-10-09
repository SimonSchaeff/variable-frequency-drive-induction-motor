/*
 * pwmFunc.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 *
 *  3-Phase SPWM (Sinusoidal PWM) for 6-switch H-bridge inverter
 *
 *  TIM3 Center-Aligned counter: -32768 → +32767 → -32768 (Q0.15)
 *
 *  Carrier signals (Q0.15, normalized -1..+1):
 *    u_a =  A * sin(2π*f*t)
 *    u_b =  A * sin(2π*f*t - 2π/3)
 *    u_c =  A * sin(2π*f*t - 4π/3)
 *
 *  PWM rule: phase HIGH if sine > carrier, LOW if sine < carrier
 *
 *  Output pins (gate driver handles inverse signal + dead-time):
 *    Phase A: PB10
 *    Phase B: PB4
 *    Phase C: PB3
 */

#ifndef INC_PWMFUNC_H_
#define INC_PWMFUNC_H_

#include "stm32f1xx_hal.h"
#include "mathFunc.h"


void spwmUpdate(void); /* call every 100µs in task4 */
void spwmBrake(void); /* call by button isr */

#endif /* INC_PWMFUNC_H_ */
