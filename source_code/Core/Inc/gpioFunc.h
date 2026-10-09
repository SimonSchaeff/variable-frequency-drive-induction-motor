/*
 * gpioFunc.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

#ifndef INC_GPIOFUNC_H_
#define INC_GPIOFUNC_H_

#include "stm32f1xx_hal.h"
#include "main.h"

/* ============================================================
 * Nucleo-F103RB – all blue pins, named directly after MCU pin name
 *  PA13/PA14 = SWDIO/SWCLK – do not use while debugging!
 * ============================================================ */

/* --- Port A --- */
#define PA0_Pin     GPIO_PIN_0
#define PA0_Port    GPIOA
#define PA1_Pin     GPIO_PIN_1
#define PA1_Port    GPIOA
#define PA2_Pin     GPIO_PIN_2
#define PA2_Port    GPIOA
#define PA3_Pin     GPIO_PIN_3
#define PA3_Port    GPIOA
#define PA4_Pin     GPIO_PIN_4
#define PA4_Port    GPIOA
#define PA5_Pin     GPIO_PIN_5
#define PA5_Port    GPIOA
#define PA6_Pin     GPIO_PIN_6
#define PA6_Port    GPIOA
#define PA7_Pin     GPIO_PIN_7
#define PA7_Port    GPIOA
#define PA8_Pin     GPIO_PIN_8
#define PA8_Port    GPIOA
#define PA9_Pin     GPIO_PIN_9
#define PA9_Port    GPIOA
#define PA10_Pin    GPIO_PIN_10
#define PA10_Port   GPIOA
#define PA11_Pin    GPIO_PIN_11
#define PA11_Port   GPIOA
#define PA12_Pin    GPIO_PIN_12
#define PA12_Port   GPIOA
#define PA15_Pin    GPIO_PIN_15
#define PA15_Port   GPIOA

/* --- Port B --- */
#define PB0_Pin     GPIO_PIN_0
#define PB0_Port    GPIOB
#define PB1_Pin     GPIO_PIN_1
#define PB1_Port    GPIOB
#define PB2_Pin     GPIO_PIN_2
#define PB2_Port    GPIOB
#define PB3_Pin     GPIO_PIN_3
#define PB3_Port    GPIOB
#define PB4_Pin     GPIO_PIN_4
#define PB4_Port    GPIOB
#define PB5_Pin     GPIO_PIN_5
#define PB5_Port    GPIOB
#define PB6_Pin     GPIO_PIN_6
#define PB6_Port    GPIOB
#define PB7_Pin     GPIO_PIN_7
#define PB7_Port    GPIOB
#define PB8_Pin     GPIO_PIN_8
#define PB8_Port    GPIOB
#define PB9_Pin     GPIO_PIN_9
#define PB9_Port    GPIOB
#define PB10_Pin    GPIO_PIN_10
#define PB10_Port   GPIOB
#define PB11_Pin    GPIO_PIN_11
#define PB11_Port   GPIOB
#define PB12_Pin    GPIO_PIN_12
#define PB12_Port   GPIOB
#define PB13_Pin    GPIO_PIN_13
#define PB13_Port   GPIOB
#define PB14_Pin    GPIO_PIN_14
#define PB14_Port   GPIOB
#define PB15_Pin    GPIO_PIN_15
#define PB15_Port   GPIOB

/* --- Port C --- */
#define PC0_Pin     GPIO_PIN_0
#define PC0_Port    GPIOC
#define PC1_Pin     GPIO_PIN_1
#define PC1_Port    GPIOC
#define PC2_Pin     GPIO_PIN_2
#define PC2_Port    GPIOC
#define PC3_Pin     GPIO_PIN_3
#define PC3_Port    GPIOC
#define PC4_Pin     GPIO_PIN_4
#define PC4_Port    GPIOC
#define PC5_Pin     GPIO_PIN_5
#define PC5_Port    GPIOC
#define PC6_Pin     GPIO_PIN_6
#define PC6_Port    GPIOC
#define PC7_Pin     GPIO_PIN_7
#define PC7_Port    GPIOC
#define PC8_Pin     GPIO_PIN_8
#define PC8_Port    GPIOC
#define PC9_Pin     GPIO_PIN_9
#define PC9_Port    GPIOC
#define PC10_Pin    GPIO_PIN_10
#define PC10_Port   GPIOC
#define PC12_Pin    GPIO_PIN_12
#define PC12_Port   GPIOC
#define PC13_Pin    GPIO_PIN_13
#define PC13_Port   GPIOC
#define PC14_Pin    GPIO_PIN_14
#define PC14_Port   GPIOC
#define PC15_Pin    GPIO_PIN_15
#define PC15_Port   GPIOC

/* --- Port D --- */
#define PD0_Pin     GPIO_PIN_0
#define PD0_Port    GPIOD
#define PD1_Pin     GPIO_PIN_1
#define PD1_Port    GPIOD

/* --- Custom names --- */
#define LED0_Pin        PB5_Pin
#define LED0_GPIO_Port  PB5_Port

#define BTN1_Pin        PC9_Pin
#define BTN1_GPIO_Port  PC9_Port
#define BTN1  0
#define BTN1_IRQ_PRIORITY  0 /* Interrupt priority for BTN1 (0 = highest, 15 = lowest) */

#define ITRIP_Pin       PC5_Pin
#define ITRIP_GPIO_Port PC5_Port

#define RELAY_PIN      PC4_Pin
#define RELAY_PORT     PC4_Port

/* --- Exported variables --- */

extern volatile bool brakeActive;

/* --- Exported functions --- */
void gpioInit(void);

void gpioTogglePin(GPIO_TypeDef *port, uint16_t pin);
void gpioSetPin(GPIO_TypeDef *port, uint16_t pin, int on);

#endif /* INC_GPIOFUNC_H_ */
