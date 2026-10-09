/*
 * application.h
 *
 *  Project:    Hochschule Muenchen
 *  Created on: 2026-03-28
 *  Author:     Simon Schäffler
 */

#ifndef INC_APPLICATION_H_
#define INC_APPLICATION_H_

void setup(void);
void loop(void);

/* Task functions – called by the scheduler */
void task1(void);
void task2(void);
void task3(void);
void task4(void);

#endif /* INC_APPLICATION_H_ */
