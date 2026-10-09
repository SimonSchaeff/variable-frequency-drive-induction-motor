/*
 * sevSegFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-04-01
 *  Author:     Simon Schäffler
 */

#include "stdint.h"
#include "main.h"

void sevSegDigitOut(int which, int segments);

/* ============ Variables ============ */

/* Segment lookup table */
uint8_t sevSegLUT[]=
{
    0x3f,    /* "0" */
	0x6, 	 /* "1" */
	0x5B, 	 /* "2" */
	0x4F, 	 /* "3" */
	0x66, 	 /* "4" */
	0x6D, 	 /* "5" */
	0x7D, 	 /* "6" */
	0x7, 	 /* "7" */
	0x7F, 	 /* "8" */
	0x6F, 	 /* "9" */
	0x77, 	 /* "A" */
	0x7C, 	 /* "b" */
	0x39, 	 /* "C" */
	0x5E, 	 /* "d" */
	0x79, 	 /* "E" */
	0x71, 	 /* "F" */
};
uint8_t digitContent[2] = {0, 0};  /* display off */

int curDigit;

/* ============ Defines ============ */

#define SEVA_Pin GPIO_PIN_6
#define SEVA_GPIO_Port GPIOC

#define SEVB_Pin GPIO_PIN_6
#define SEVB_GPIO_Port GPIOB

#define SEVC_Pin GPIO_PIN_2
#define SEVC_GPIO_Port GPIOC

#define SEVD_Pin GPIO_PIN_3
#define SEVD_GPIO_Port GPIOC

#define SEVE_Pin GPIO_PIN_1
#define SEVE_GPIO_Port GPIOC

#define SEVF_Pin GPIO_PIN_0
#define SEVF_GPIO_Port GPIOC

#define SEVG_Pin GPIO_PIN_8
#define SEVG_GPIO_Port GPIOC

#define SEVCOM_Pin GPIO_PIN_0
#define SEVCOM_GPIO_Port GPIOB

GPIO_TypeDef *SevSegPort[] =
{
    SEVA_GPIO_Port,
	SEVB_GPIO_Port,
	SEVC_GPIO_Port,
	SEVD_GPIO_Port,
	SEVE_GPIO_Port,
	SEVF_GPIO_Port,
	SEVG_GPIO_Port,
};
int SevSegPin[] =
{
	SEVA_Pin,
	SEVB_Pin,
	SEVC_Pin,
	SEVD_Pin,
	SEVE_Pin,
	SEVF_Pin,
	SEVG_Pin,
};

/* ============ Functions ============ */

void sevSegTask(void)
{
	sevSegDigitOut(curDigit & 1, digitContent[curDigit & 1]);
	curDigit++;
}

/*
 * value: decimal number 0 - 99
 */
void sevSegDecOut(int value)
{
	value = value % 100;                       /* modulo 100: keep only two digits */
	digitContent[0] = sevSegLUT[value % 10];   /* ones digit  */
	digitContent[1] = sevSegLUT[value / 10];   /* tens digit  */
}

/*
 * value: decimal number 0 - 255  ( 0-FF)
 */
void sevSegHexOut(int value)
{
	value = value & 0xFF;
	digitContent[0] = sevSegLUT[value & 0x0F];   /* ones digit (lower nibble)  */
	digitContent[1] = sevSegLUT[value >> 4];     /* sixteens digit (upper nibble) */
}

void sevSegInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

	/* SEG A (PC6), C (PC2), D (PC3), E (PC1), F (PC0), G (PC8) → GPIOC */
	GPIO_InitStruct.Pin = SEVA_Pin | SEVC_Pin | SEVD_Pin | SEVE_Pin | SEVF_Pin | SEVG_Pin;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* SEG B (PB6), COM (PB0) → GPIOB */
	GPIO_InitStruct.Pin = SEVB_Pin | SEVCOM_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/*
 * parameter which: 0: right digit 1: left digit
 * parameter segments: bit coded: Bit 0 -> Seg a, Bit 1 -> Seg B etc
 */
void sevSegDigitOut(int which, int segments)
{
	int curSeg;
	HAL_GPIO_WritePin(SEVCOM_GPIO_Port, SEVCOM_Pin, which);
	if (which)
	{
		/* Left digit: common anode → 0 turns on, 1 turns off. Invert the segments bitmask. */
		segments = ~segments;
	}
	for (curSeg = 0; curSeg < 7; curSeg++)
	{
		HAL_GPIO_WritePin(SevSegPort[curSeg], SevSegPin[curSeg], segments & (1 << curSeg));
	}

}


