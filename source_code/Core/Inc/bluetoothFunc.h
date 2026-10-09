/*
 * bluetoothFunc.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-04-26
 *  Author:     Simon Schäffler
 *
 *  USART3 – simple polling communication (no interrupt)
 *
 *  TX: PB10  (AF Push-Pull)
 *  RX: PB11  (Input Floating)
 *
 *  Test setup with PC:
 *    PB10 (TX) ---► RX pin of USB-TTL adapter  (e.g. CP2102 / CH340)
 *    PB11 (RX) ◄--- TX pin of USB-TTL adapter
 *    GND       ---► GND of adapter
 *    → PC: e.g. PuTTY / Tera Term, 9600 Baud, 8N1
 *
 *  WARNING DSUB9 (RS232):
 *    RS232 uses +-12V levels – do NOT connect directly to the MCU!
 *    A level shifter (e.g. MAX232) or a USB-TTL adapter (CP2102 / CH340)
 *    with 3.3V/5V TTL is required.
 */

#ifndef INC_BLUETOOTHFUNC_H_
#define INC_BLUETOOTHFUNC_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ============ Pin and USART Configuration ============ */
#define BT_TX_Pin        GPIO_PIN_10
#define BT_TX_GPIO_Port  GPIOB

#define BT_RX_Pin        GPIO_PIN_11
#define BT_RX_GPIO_Port  GPIOB

#define BT_USART         USART3
#define BT_BAUD          9600

/* Timeout only needed for TX (transmit) – RX runs via DMA */
#define BT_TIMEOUT_MS    20

/* DMA ring buffer size (must be a power of 2, minimum 16) */
#define BT_DMA_BUF_SIZE  64

/* ============ API ============ */

/** Initialises USART3. Call once in setup(). */
void btInit(void);

/**
 * Sends a null-terminated string.
 */
void btSend(const char *str);

/**
 * Receives a number (e.g. "42\n").
 * Returns the received number, -1 if nothing available (non-blocking).
 * -2 if input is not a valid numeric value.
 */
int btReceiveNumber(void);

uint8_t btmain();

#endif /* INC_BLUETOOTHFUNC_H_ */
