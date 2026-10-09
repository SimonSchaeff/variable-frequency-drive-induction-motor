/*
 * bluetoothFunc.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-04-26
 *  Author:     Simon Schäffler
 *
 *  USART3 – simple polling communication (no interrupt)
 *    TX: PB10  (AF Push-Pull)
 *    RX: PB11  (Input Floating)
 *
 *  Test setup with PC:
 *    MCU PB10 (TX) ──► RX  of USB-TTL adapter (CP2102 / CH340)
 *    MCU PB11 (RX) ◄── TX  of USB-TTL adapter
 *    MCU GND       ──► GND of USB-TTL adapter
 *    PC: PuTTY / Tera Term  →  9600 Baud, 8N1, no flow control
 *
 *  WARNING: DSUB9 uses RS232 levels (+-12V) → MAX232 level shifter required!
 *  Simpler: use a USB-TTL adapter (3.3V) directly.
 */

#include "main.h"
#include "bluetoothFunc.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* ============ Private Variables ============ */

static UART_HandleTypeDef s_huart3;
uint8_t freq_bt = 0;
bool StartUpvar = true;

/* --- DMA ring buffer --- */
static uint8_t  s_dmaBuf[BT_DMA_BUF_SIZE];  /* DMA writes here in circular mode */
static uint16_t s_readIdx = 0;               /* next byte to read                */

/* Returns the number of bytes available in the ring buffer */
static uint16_t ringAvail(void)
{
    /* DMA1 Channel3 CNDTR counts down: writeIdx = BufSize - CNDTR */
    uint16_t writeIdx = BT_DMA_BUF_SIZE - (uint16_t)DMA1_Channel3->CNDTR;
    if (writeIdx >= s_readIdx)
        return writeIdx - s_readIdx;
    return (uint16_t)(BT_DMA_BUF_SIZE - s_readIdx + writeIdx);
}

/* Reads one byte from the ring buffer */
static uint8_t ringGet(void)
{
    uint8_t b  = s_dmaBuf[s_readIdx];
    s_readIdx  = (s_readIdx + 1) % BT_DMA_BUF_SIZE;
    return b;
}

/* ============ Public Functions ============ */

/*
 * btInit()
 * Initialises USART3 at 9600 Baud, 8N1, RX via DMA circular mode.
 */
void btInit(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* --- Enable clocks --- */
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* --- TX: PB10 – Alternate Function Push-Pull --- */
    gpio.Pin   = BT_TX_Pin;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BT_TX_GPIO_Port, &gpio);

    /* --- RX: PB11 – Input Floating --- */
    gpio.Pin  = BT_RX_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BT_RX_GPIO_Port, &gpio);

    /* --- Configure USART3 --- */
    s_huart3.Instance          = BT_USART;
    s_huart3.Init.BaudRate     = BT_BAUD;
    s_huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    s_huart3.Init.StopBits     = UART_STOPBITS_1;
    s_huart3.Init.Parity       = UART_PARITY_NONE;
    s_huart3.Init.Mode         = UART_MODE_TX_RX;
    s_huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&s_huart3);

    /* --- DMA1 Channel3: USART3_RX in Circular Mode --- */
    __HAL_RCC_DMA1_CLK_ENABLE();

    DMA1_Channel3->CCR  = 0;                          /* disable first           */
    DMA1_Channel3->CPAR = (uint32_t)&USART3->DR;      /* peripheral: USART3      */
    DMA1_Channel3->CMAR = (uint32_t)s_dmaBuf;         /* memory: ring buffer     */
    DMA1_Channel3->CNDTR= BT_DMA_BUF_SIZE;            /* number of bytes         */
    DMA1_Channel3->CCR  = DMA_CCR_CIRC  |             /* circular mode           */
                          DMA_CCR_MINC;                /* memory increment        */
    /* DIR=0 (Periph→Mem), PSIZE/MSIZE = 8bit (default) */
    DMA1_Channel3->CCR |= DMA_CCR_EN;                 /* enable DMA              */

    USART3->CR3 |= USART_CR3_DMAR;                    /* enable USART3 DMA-RX    */
}

/*
 * btSend()
 * Sends a null-terminated string in blocking mode.
 * Returns once all bytes have been transmitted (max. BT_TIMEOUT_MS).
 */
void btSend(const char *str)
{
    HAL_UART_Transmit(&s_huart3, (const uint8_t *)str, (uint16_t)strlen(str), BT_TIMEOUT_MS);
}

/*
 * btReceiveNumber()
 * Reads from the DMA ring buffer – completely non-blocking.
 * Returns the received number, -1 if nothing available, -2 for invalid input.
 */
int btReceiveNumber(void)
{
    if (ringAvail() == 0) return -1;   /* nothing received */

    char    buf[8];
    uint8_t i = 0;

    /* Read bytes from ring buffer until newline or buffer full */
    while (ringAvail() > 0 && i < sizeof(buf) - 1)
    {
        uint8_t b = ringGet();
        if (b == '\r' || b == '\n') break;
        buf[i++] = (char)b;
    }
    buf[i] = '\0';

    if (i == 0) return -1;

    char *endptr;
    long val = strtol(buf, &endptr, 10);
    if (endptr == buf) return -2;   /* not a valid number */

    return (int)val;
}

uint8_t btmain()
{
    /* start up message */
    if (StartUpvar == true)
    {
        btSend("start up\r\n");
	    btSend("select 0Hz ... 99Hz \r\n");
        StartUpvar = false; /* run only once */
    }
    
    /* normal process */
    int ReceivNumber = btReceiveNumber();   /* -1: nothing received, -2: invalid numeric value */
    if ((ReceivNumber >= 0) && (ReceivNumber <= 99))
    {
        freq_bt = ReceivNumber;
        btSend("accepted \r\n");
		btSend("select 0Hz ... 99Hz \r\n");
    }
	else if (ReceivNumber != -1)
	{
		btSend("invalid input \r\n");
	}
	else
	{
		/* ReceivNumber == -1 */
		/* nothing to do (there was nothing send) */
	}
    return freq_bt;
}