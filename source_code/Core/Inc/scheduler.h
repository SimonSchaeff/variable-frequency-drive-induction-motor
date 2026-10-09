/*
 * scheduler.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 *
 *  Mini Real-Time Scheduler
 *  Base: 1ms tick from TIM2
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>

/* Maximum number of tasks */
#define SCH_MAX_TASKS   4

/* Task status */
#define TASK_IDLE       0
#define TASK_READY      1

/* Task descriptor */
typedef struct {
    void        (*taskFunc)(void);  /* pointer to the task function       */
    uint32_t    period;             /* period in scheduler ticks          */
    uint32_t    counter;            /* countdown counter                  */
    uint8_t     ready;              /* set to TASK_READY when due         */
} Task_t;

/* Public API */
void schInit(void);         /* initialise the scheduler                   */
void schTick(void);         /* called from the timer ISR every 100µs      */
void schRun(void);          /* called from the superloop                  */

#endif /* INC_SCHEDULER_H_ */
