/*
 * scheduler.c
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 *
 *  Mini Real-Time Scheduler
 *  Base: 1ms tick from TIM2
 */

#include "scheduler.h"
#include "application.h"

/* Internal task table */
static Task_t taskTable[SCH_MAX_TASKS];

/* -------------------------------------------------------
 * schInit()
 * Registers the 3 tasks with their periods.
 * Called once in setup().
 * ------------------------------------------------------- */
void schInit(void)
{
    /* Task 0 – every 2000 ticks = 200ms → LED toggle */
    taskTable[0].taskFunc = task1;
    taskTable[0].period   = 2000;
    taskTable[0].counter  = 2000;
    taskTable[0].ready    = TASK_IDLE;

    /* Task 1 – every 100 ticks = 10ms → ADC read */
    taskTable[1].taskFunc = task2;
    taskTable[1].period   = 100;
    taskTable[1].counter  = 100;
    taskTable[1].ready    = TASK_IDLE;

    /* Task 2 – every 10 ticks = 1ms → free for use */
    taskTable[2].taskFunc = task3;
    taskTable[2].period   = 10;
    taskTable[2].counter  = 10;
    taskTable[2].ready    = TASK_IDLE;

    /* Task 3 – every 1 tick = 100µs → PWM control */
    taskTable[3].taskFunc = task4;
    taskTable[3].period   = 1;
    taskTable[3].counter  = 1;
    taskTable[3].ready    = TASK_IDLE;
}

/* -------------------------------------------------------
 * schTick()
 * Called from the timer ISR every 1 ms.
 * Counts down counters and marks tasks as READY.
 * Keep short – do NOT call task functions here!
 * ------------------------------------------------------- */
void schTick(void)
{
    uint8_t i;
    for (i = 0; i < SCH_MAX_TASKS; i++)
    {
        if (taskTable[i].counter > 0)
        {
            taskTable[i].counter--;
        }
        if (taskTable[i].counter == 0)
        {
            taskTable[i].ready   = TASK_READY;
            taskTable[i].counter = taskTable[i].period; /* reload */
        }
    }
}

/* -------------------------------------------------------
 * schRun()
 * Called in the superloop (loop()).
 * Executes all READY tasks one after another
 * (cooperative multitasking).
 * ------------------------------------------------------- */
void schRun(void)
{
    uint8_t i;
    for (i = 0; i < SCH_MAX_TASKS; i++)
    {
        if (taskTable[i].ready == TASK_READY)
        {
            taskTable[i].ready = TASK_IDLE;     /* reset before call */
            taskTable[i].taskFunc();             /* execute task      */
        }
    }
}
