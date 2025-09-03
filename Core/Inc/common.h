/*
 * common.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: If you feel that there are common
 *      C functions corresponding to this
 *      header, then any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_


// Definitions
#define MSP_STACK_SIZE 0x4000

#define TID_NULL 0 //predefined Task ID for the NULL task
#define MAX_TASKS 16 //maximum number of tasks in the system
#define STACK_SIZE 0x200 //min. size of each task's stack
#define DORMANT 0 //state of terminated task
#define READY 1 //state of task that can be scheduled but is not running
#define RUNNING 2 //state of running task
#define SLEEPING 3 //state of sleeping task - lab 3
#define RTX_OK 0
#define RTX_ERR -1
#define RTX_NULL ((void*)0)
#define UNINT_TASK_ID 16

// SVC call numbers
#define SVC_LOAD_PSP 1
#define SVC_KERNEL_INIT 57
#define SVC_CREATE_TASK 58
#define SVC_KERNEL_START 59
#define SVC_YIELD 60        
#define SVC_GET_TID 61
#define SVC_TASK_INFO 62
#define SVC_TASK_EXIT 63
#define SVC_MEM_INIT 64
#define SVC_MEM_ALLOC 65
#define SVC_MEM_DEALLOC 66
#define SVC_COUNT_EXT 67
#define SVC_CREATE_DEAD_TASK 68

/* Type definitions */
typedef unsigned int U32;
typedef unsigned short U16;
typedef char U8;
typedef unsigned int task_t;

/* Structs */
typedef struct task_control_block{
    U32 * psp_stack_ptr;         //stores the PSP
    U32 stack_high;              //stack high pointer
    void (*ptask)(void* args);   //entry address of task to be executed
    task_t tid;                  //task ID
    U8 state;                    //task's state
    U16 stack_size;              //stack size. Must be a multiple of 8

    /* lab 3*/
    int deadline;                //deadline in ms
    int time_remaining;          //time remaining in current timeslice, in ms
    int sleep_time;              //time left to sleep, in ms

} TCB;

#endif /* INC_COMMON_H_ */
