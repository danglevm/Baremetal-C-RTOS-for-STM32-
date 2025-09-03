/*
 * k_mem.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */
#include <stddef.h>
#include "stm32f4xx.h"
#include "k_task.h"
#include "common.h"

#ifndef INC_K_MEM_H_
#define INC_K_MEM_H_
#define MAGIC_NUMBER 0xCAFEBABE
#define KERNEL_TID 0xDEADBEEF

/* Function declarations */
int k_mem_init(void);
void* k_mem_alloc(size_t size);
int k_mem_dealloc(void* ptr);
int k_mem_count_extfrag(size_t size);

// Declare struct for memory block (20 bytes)
typedef struct mem_blk {
    size_t size;            // Size of the memory block
    int is_free;            // 1: free, 0: allocated
    task_t owner_TID;       // TID of the task that owns this block, TID_NULL if not owned
    struct mem_blk* next;   // Pointer to the next block
    U32 magic;              // 0xCAFEBABE
} mem_blk_t;

#endif /* INC_K_MEM_H_ */
