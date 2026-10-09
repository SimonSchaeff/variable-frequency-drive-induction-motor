/*
 * adFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

/* ============ Includes ============ */
#include "main.h"
#include "adFunc.h"

/* ============ Defines ============ */
#define ADCERROR 0xFFFF

/* Indices into adres[] DMA buffer (array to hold AD values) */
#define AD_CH_PA0 0      /* poti for preferred frequency  */
#define AD_CH_PA1 1      /* temperature measurement       */
#define AD_CH_PA4 2      /* phase current U               */
#define AD_CH_PA5 3      /* phase current V               */
#define AD_CH_PA6 4      /* phase current W               */
#define AD_NUM_CH 5

#define ADC_VREF          3.3f    /* [V]       */
#define ADC_RESOLUTION    4095.0f /* 12-bit ADC */

#define ADC_POTI_RESULUTION 4035.0f /* poti does not reach full scale 4095 */
#define MAX_POTI_VAL 99



/* Preliminary Calculation for Equation Factor: Current Calculation
 * Based on Voltage at the uC Pin
 *
 * Circuit:  I --> R_SHUNT --> OpAmp (gain=AMP_FAC) --> Voltage Divider --> uC Pin
 *
 * U_shunt   = I * R_SHUNT
 * U_amp     = U_shunt * AMP_FAC
 * U_pin     = U_amp * R2/(R1+R2)   <-- voltage divider attenuates back into ADC range
 *
 * Solved for I:
 *   I = U_pin * (R1+R2)/R2 / (R_SHUNT * AMP_FAC)
 *   I = CALC_FAC * U_pin
 *
 * ADC resolution: 12-bit (0..4095) mapped to 0..3.3 V
 *   U_pin = adcRaw * (3.3 / 4095)
 */
#define R_SHUNT               0.003f  /* [ohm]  shunt resistor                         */
#define AMP_FAC               100.0f  /* amplification factor of current sense amplifier */
#define VOLT_DIV_R1           3900.0f /* [ohm]  upper resistor of voltage divider        */
#define VOLT_DIV_R2           6670.0f /* [ohm]  lower resistor of voltage divider        */
#define ADC_ZERO_OFFSET_CUR   2880.0f

/* VOLT_DIV_RATIO = (R1+R2)/R2 = 10570/6670 ≈ 1.585  (floating-point!) */
#define VOLT_DIV_RATIO    ((VOLT_DIV_R1 + VOLT_DIV_R2) / VOLT_DIV_R2)
/* CALC_FAC ≈ 5.28 A/V */
#define CALC_FAC          (VOLT_DIV_RATIO / (R_SHUNT * AMP_FAC))
/* I [A] = CALC_FAC * U_pin [V] */

/* values for temperature measurement over VFO pin of gate driver module IM523-X6A*/
#define PULLUP_R 33000.0f /* [ohm]              */
#define TEMP_MAX 110      /* [degrees Celsius]  */

/* ============ Variables ============ */
static uint16_t adres[AD_NUM_CH];

/* IM523-X6A Datasheet NTC Values (page 13, "Rtyp") – original table (unused entries kept for reference):
	// {250000, 0},
	// {85000,  25},
    // {29972,  50},
    // {20515,  60},
    // {14315,  70},
    // {10188,  80},
    // {7345,   90},
    // {5388,   100},
    // {4009,   110},
    // {3024,   120},
    // {2639,   125}
*/
typedef struct {
    uint32_t resistance; /* [ohm]              */
    uint8_t temperature; /* [degrees Celsius]  */
} NtcEntry_t;

const NtcEntry_t NtcLut[] = {

	{256712, 0},
	{165088, 10},
	{132218, 15},
	{105946, 20},
	{85035,  25},
	{68448,  30},
	{55321,  35},
	{44939,  40},
	{36714,  45},
	{30162,  50},
	{24886,  55},
	{20552,  60},
	{16875,  65},
	{13601,  70},
	{10486,  75},
	{7345,   90},
	{5388,   100},
	{4009,   110},
	{3024,   120},
	{2639,   125}
};

#define NTCLUT_SIZE (sizeof(NtcLut) / sizeof(NtcLut[0]))

ADC_HandleTypeDef adcHandle;
DMA_HandleTypeDef hdma_adc1;

/* ============ Functions ============ */

void ADInit(void)
{
    RCC_PeriphCLKInitTypeDef  PeriphClkInit;
    GPIO_InitTypeDef AD_IO_Init;
    ADC_ChannelConfTypeDef sConfig;

    __ADC1_CLK_ENABLE();
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    /* activate and configure ADC clock */
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    /* configure analog GPIO pins */
    AD_IO_Init.Mode = GPIO_MODE_ANALOG;
	AD_IO_Init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
	AD_IO_Init.Pull = GPIO_NOPULL;
	AD_IO_Init.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &AD_IO_Init);

	/* configure ADC */
	adcHandle.Instance = ADC1;
	adcHandle.Init.ContinuousConvMode = ENABLE;
	adcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	adcHandle.Init.DiscontinuousConvMode = DISABLE;
	adcHandle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	adcHandle.Init.NbrOfConversion = 5;
	adcHandle.Init.NbrOfDiscConversion = 0;
	adcHandle.Init.ScanConvMode = ADC_SCAN_ENABLE;
	HAL_ADC_Init(&adcHandle);

	sConfig.Channel = ADC_CHANNEL_0;   /* PA0 – Poti */
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
	HAL_ADC_ConfigChannel(&adcHandle, &sConfig);
	sConfig.Channel = ADC_CHANNEL_1;   /* PA1 */
	sConfig.Rank = ADC_REGULAR_RANK_2;
	HAL_ADC_ConfigChannel(&adcHandle, &sConfig);
	sConfig.Channel = ADC_CHANNEL_4;   /* PA4 */
	sConfig.Rank = ADC_REGULAR_RANK_3;
	HAL_ADC_ConfigChannel(&adcHandle, &sConfig);
	sConfig.Channel = ADC_CHANNEL_5;   /* PA5 */
	sConfig.Rank = ADC_REGULAR_RANK_4;
	HAL_ADC_ConfigChannel(&adcHandle, &sConfig);
	sConfig.Channel = ADC_CHANNEL_6;   /* PA6 */
	sConfig.Rank = ADC_REGULAR_RANK_5;
	HAL_ADC_ConfigChannel(&adcHandle, &sConfig);
    HAL_ADC_Start(&adcHandle);

	__HAL_RCC_DMA1_CLK_ENABLE();
    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&adcHandle, DMA_Handle, hdma_adc1);
	HAL_ADC_Start_DMA(&adcHandle, (void*)adres, 5);  /* start ADC with DMA */
}

/*
 * potiToFreqHz()
 * Maps ADC raw value 0..4095 to output frequency 0..100 Hz.
 */
uint16_t potiToFreqHz()
{
	uint16_t rawValue = adres[AD_CH_PA0];   /* ADC raw value of Poti */
	uint16_t value = (uint16_t)((uint32_t)(rawValue) * 100UL / ADC_POTI_RESULUTION);
	if (value > MAX_POTI_VAL)
	{
		value = MAX_POTI_VAL;
	}
    return value;
}

/*
 * getPhaseCurrents()
 * Reads the ADC values for phases U, V, W and converts them to currents [A].
 *
 * The ADC raw value (0..4095) is first converted to a voltage (0..3.3 V),
 * then the current is derived using the pre-calculated CALC_FAC:
 *   I [A] = CALC_FAC * (adcRaw * ADC_VREF / ADC_RESOLUTION)
 *
 * Result is stored in the PhaseCurrents_t struct passed by pointer.
 */
void getPhaseCurrents(PhaseCurrents_t *currents)
{
    currents->I_U = CALC_FAC * ((((float)adres[AD_CH_PA4]) - ADC_ZERO_OFFSET_CUR) * ADC_VREF / ADC_RESOLUTION);
    currents->I_V = CALC_FAC * ((((float)adres[AD_CH_PA5]) - ADC_ZERO_OFFSET_CUR) * ADC_VREF / ADC_RESOLUTION);
    currents->I_W = CALC_FAC * ((((float)adres[AD_CH_PA6]) - ADC_ZERO_OFFSET_CUR) * ADC_VREF / ADC_RESOLUTION);
}

volatile float ntcValue;
/* getVfoValue*/
void getVfoValue(VFOInfo_t *GateDriverState)
{
	uint16_t rawValue = adres[AD_CH_PA1];
	uint8_t Temp;
	if (rawValue < 20)
	{
		/* gate driver close mosfet due to emergency stop
		 * VDO is now on GND potential --> set emergency flag
		 */
		GateDriverState->Failure = true;
	}
	else
	{
		/* gate driver has no emergency stop set */
		GateDriverState->Failure = false;
		/* normal temp meas mode --> determine ntc value and temperature */
		float uMeas = (float)rawValue * ADC_VREF / ADC_RESOLUTION;
		/* Voltage divider (pull-up on top, NTC to GND):
		 * R_NTC = PULLUP_R * uMeas / (ADC_VREF - uMeas) */
		ntcValue = (PULLUP_R * uMeas) / (ADC_VREF - uMeas);
		/* LUT is sorted descending by resistance (0°C = highest R).
		 * Iterate from coldest entry (index 0) to hottest.
		 * First entry where ntcValue >= resistance gives the temperature.
		 * (Higher resistance = lower temp, so we scan from large R down.) */
		Temp = NtcLut[NTCLUT_SIZE - 1].temperature; /* default: hottest entry (NTC very hot) */
		for (uint8_t i = 0; i < NTCLUT_SIZE; i++)
		{
			if (ntcValue >= NtcLut[i].resistance)
			{
				Temp = NtcLut[i].temperature;
				break;
			}
		}
		GateDriverState->Temperature = Temp;
		if (Temp > TEMP_MAX)
		{
			/* in case of over temperature set Failure-flag true */
			GateDriverState->Failure = true;
		}
	}
}