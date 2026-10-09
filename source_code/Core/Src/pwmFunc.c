/*
 * pwmFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 *
 *  3-Phase SPWM for 6-switch inverter
 *  TIM3 Center-Aligned counter used as carrier signal
 *
 *  Output pins (gate driver handles inverse signal + dead-time):
 *    Phase A: PB10
 *    Phase B: PB4
 *    Phase C: PB3
 */

/* ============ Includes ============ */
#include "main.h"
#include "pwmFunc.h"
#include "gpioFunc.h"
#include "freqFunc.h"
#include "config.h"         /* operating mode: CFG_MODE_LAB / CFG_MODE_MAINS */

/* ============ Defines ============ */

/* --- Derived from config.h --- */
#define U_ZK_HALF     (CFG_U_ZK / 2)                                      /* [V]    DC link half voltage                       */
#define K_UF          ((float)CFG_U_NENN / (float)CFG_F_NENN)             /* [V/Hz] U/f slope (e.g. k = 230V / 50Hz = 4.6)     */
#define MOD_FACTOR    ((uint32_t)((K_UF * 32767.0f) / (float)U_ZK_HALF))  /* Q0.15 factor per Hz MOD_FACTOR = (k / (U_ZK / 2)) */
#define F_MAX         ((uint16_t)((float)U_ZK_HALF / K_UF))               /* [Hz]   max freq without overmod (m = 1)           */

/* 120° and 240° in Q0.15 angle format */
#define ANGLE_120DEG  (frac16_t)0x5555              /* 120° */
#define ANGLE_240DEG  (frac16_t)((sint16)0xAAAA)    /* 240° */

/* ============ Forward Declarations ============ */

static frac16_t  freqToAngleStep(uint16_t freqHz);
static frac16_t  freqToModIndex(uint16_t freqHz);

/* ============ Variables ============ */

static frac16_t s_angleStep = 0;   /* updated every 100µs in spwmUpdate() */
static frac16_t s_angle     = 0;                 /* current angle  */

/* Modulation index: 0x7FFF = 1.0 (max), 0x4000 = 0.5 */
static frac16_t s_modIndex  = 0;   /* updated every 100µs in spwmUpdate() */

/* Debug variables - visible in debugger */
volatile frac16_t spwm_u_a     = 0;
volatile frac16_t spwm_u_b     = 0;
volatile frac16_t spwm_u_c     = 0;
volatile sint16   spwm_carrier = 0;
/* volatile uint32_t taskCountpwmUpdate = 0; */

/* ============ Local Functions ============ */


/*
 * freqToAngleStep()
 * Converts Hz to Q0.15 angle increment per carrier-peak tick.
 * spwmUpdate() is called from TIM1 ISR at 5 kHz (72MHz / (2*7200)).
 * 2 * 'pi' = 65536 in Q0.15
 * angleStep = f * 65536 / 5000
 * Example: 50 Hz -> 655,  100 Hz -> 1311
 */
static frac16_t freqToAngleStep(uint16_t freqHz)
{
    return (frac16_t)((uint32_t)freqHz * 65536UL / 5000UL);
}

/*
 * freqToModIndex()
 *
 * U/f = const:  k = U_nenn / f_nenn  [V/Hz]  (see config.h)
 *               U_ref = k * f  [V]
 *
 * modIndex = U_ref / (U_ZK / 2)
 *
 * As Q0.15:
 *   modIndex_Q0.15 = f * MOD_FACTOR
 *
 * Overmodulation above F_MAX Hz -> clamp to 32767
 */
static frac16_t freqToModIndex(uint16_t freqHz)
{
    uint32_t mod = (uint32_t)freqHz * MOD_FACTOR;
    if (mod > 32767UL) mod = 32767UL;   /* clamp above F_MAX Hz */
    return (frac16_t)mod;
}


/* ============ Exported Functions ============ */

/*
 * spwmUpdate()
 * Called from TIM1 Update ISR at every carrier PEAK (CNT == ARR = 3600).
 * TIM1 Center-Aligned Mode 1 + URS=1 → ISR fires only at overflow (peak), 10 kHz.
 *
 * Writes CCR1/2/3 → TIM1 hardware comparator sets pins automatically:
 *   CNT < CCR  → CHx HIGH,  CHxN LOW
 *   CNT > CCR  → CHx LOW,   CHxN HIGH  (+ hardware deadtime ~1µs)
 *
 *   CH1/CH1N (PA8/PB13)  → Phase U
 *   CH2/CH2N (PA9/PB14)  → Phase V
 *   CH3/CH3N (PA10/PB15) → Phase W
 *
 * CCR = Period/2 + u * Period/2 / 32767
 *     = 1800 + u * 1800 / 32767
 */
void spwmUpdate(void)
{
    if (!brakeActive)
    {
        /* --- Update angle step and modulation index --- */
        s_angleStep = freqToAngleStep(freq_motor);
        s_modIndex  = freqToModIndex(freq_motor);

        /* --- Compute sine reference signals (Q0.15) --- */
        spwm_u_a = (frac16_t)(((int32_t)s_modIndex * MathF16_Sin(s_angle))                >> 15);
        spwm_u_b = (frac16_t)(((int32_t)s_modIndex * MathF16_Sin(s_angle - ANGLE_120DEG)) >> 15);
        spwm_u_c = (frac16_t)(((int32_t)s_modIndex * MathF16_Sin(s_angle - ANGLE_240DEG)) >> 15);

        /* --- Write CCR → hardware comparator + deadtime sets all 6 pins automatically --- */
        /* TIM1->CCR1 = (uint16_t)(3600 + ((int32_t)spwm_u_a * 3600L) / 32767L); */
        /* TIM1->CCR2 = (uint16_t)(3600 + ((int32_t)spwm_u_b * 3600L) / 32767L); */
        /* TIM1->CCR3 = (uint16_t)(3600 + ((int32_t)spwm_u_c * 3600L) / 32767L); */

        TIM1->CCR1 = (uint16_t)(3600 + (((int32_t)spwm_u_a * 3600L) >> 15));
        TIM1->CCR2 = (uint16_t)(3600 + (((int32_t)spwm_u_b * 3600L) >> 15));
        TIM1->CCR3 = (uint16_t)(3600 + (((int32_t)spwm_u_c * 3600L) >> 15));

        /* --- Advance angle --- */
        s_angle += s_angleStep;
    }
    
}

void spwmBrake(void)
{
    /* CCR = 0 → CNT always >= CCR → CHx LOW, CHxN HIGH (+ hardware dead time) */
    TIM1->CCR1 = 0;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = 0;
    /* reset angle */
    s_angle      = 0;
    s_angleStep  = 0;
    /* setting frequency immediately to zero */
    freq_motor   = 0;
    freq_pref_user = 0;
}