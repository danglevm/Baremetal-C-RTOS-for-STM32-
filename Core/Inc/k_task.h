/*
 * k_task.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */


#ifndef INC_K_TASK_H_
#define INC_K_TASK_H_
#define SHPR2 (*((volatile uint32_t*)0xE000ED1C))//SVC is bits 31-28
#define SHPR3 (*((volatile uint32_t*)0xE000ED20))//SysTick is bits 31-28, and PendSV is bits 23-20

#include "common.h"
#include "k_mem.h"
#include "stm32f4xx.h"

/* Function declarations */

void osKernelInit(void);
int osCreateTask(TCB* task);
int osKernelStart(void);     // Added missing declaration
void osYield(void);
int osTaskInfo(task_t TID, TCB* task_copy);
task_t osGetTID(void);
int osTaskExit(void);        // Added missing declaration
void osSleep(int timeInMs);
void osPeriodYield();
int osSetDeadline(int deadline, task_t TID);
int osCreateDeadlineTask(int deadline, TCB* task);

/* Typedefs */
typedef enum {
    KERNEL_UNINIT = -1,
    KERNEL_INIT = 0,
    KERNEL_RUN = 1
} kernel_state;

#endif /* INC_K_TASK_H_ */
