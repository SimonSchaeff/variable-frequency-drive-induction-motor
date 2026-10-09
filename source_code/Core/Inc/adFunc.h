/*
 * adFunc.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

#ifndef INC_ADFUNC_H_
#define INC_ADFUNC_H_

#include <stdint.h>
#include "main.h"

/* Struct to hold the three phase currents [A] */
typedef struct {
    float I_U;  /* Phase current U [A] */
    float I_V;  /* Phase current V [A] */
    float I_W;  /* Phase current W [A] */
} PhaseCurrents_t;

/* Struct to hold information of VFO pin (gate driver IM523-X6A) */
typedef struct {
    float Temperature; /* Temperature of gate driver IC */
    bool Failure;      /* In case of Failure (Undervoltage of Powersupply or ITRIP = High)*/
} VFOInfo_t;

void ADInit(void);
uint16_t potiToFreqHz(void);
void getPhaseCurrents(PhaseCurrents_t *currents);
void getVfoValue(VFOInfo_t *GateDriverState);

#endif /* INC_ADFUNC_H_ */
