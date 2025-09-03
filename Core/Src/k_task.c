#include "k_task.h"

TCB task_blocks[MAX_TASKS];
TCB* task_current;
// U32 * global_stackptr;
// U32 * global_start_ptr;
U32 tasks_available = 0;
static task_t task_nextID = 1; // 0 is TID_NULL
kernel_state state = KERNEL_UNINIT;


/************* HELPER FUNCTIONS **************/
void null_task() {
	while(1);
}

int isSlotAvailable(U16 size) {
	for(int i = 0; i < MAX_TASKS; i++){
		if(task_blocks[i].state == DORMANT){
			return i;
		}
	}
	return -1;

}

int createTCB(TCB* tcb_in, int deadline){
	// Validation (accounting for null task might need to fix as per spec/check if mem allocation condn is correct)
	if(tcb_in == RTX_NULL ||  tasks_available >= (MAX_TASKS - 1)){
		return RTX_ERR;
	}

	// Initialize stack for null task at the beginning
	if (tasks_available == 0){
		void * stack_mem = k_mem_alloc(task_blocks[TID_NULL].stack_size);
		if (stack_mem == RTX_NULL){
			return RTX_ERR;
		}
		task_blocks[TID_NULL].stack_high =  (U32)((U8*)stack_mem + task_blocks[TID_NULL].stack_size);
		/*-------------------------------------------------------------------------------------*/

		U32 * cur_stackptr = (U32*)task_blocks[TID_NULL].stack_high;
		/* enable THUMB mode for the task */
		*(--cur_stackptr) = 1 << 24; 			  // xPSR
		/* stores address of c */
		*(--cur_stackptr) = (U32)task_blocks[TID_NULL].ptask; // PC
		// LR
		/* set dummy values to R4-R11*/
		for (int i = 0;  i < 14; i++) {
			*(--cur_stackptr) = 0xA;
		}
		//setting the PSP of the stack
		task_blocks[TID_NULL].psp_stack_ptr = (U32*)cur_stackptr;
		tasks_available++;
	}

	// Find space
	int space = isSlotAvailable(tcb_in->stack_size);
	if (space == -1) {
		return RTX_ERR;
	}

	// Dynamically allocate stack using k_mem_alloc - lab3
	void * stack_mem = k_mem_alloc(tcb_in->stack_size);
	if (stack_mem == RTX_NULL){
		return RTX_ERR;
	}


	/* Create TCB */
	task_blocks[space].ptask = tcb_in->ptask;
	task_blocks[space].state = READY;
	task_blocks[space].stack_size = tcb_in->stack_size;
	task_blocks[space].tid = ((task_nextID++) % 15) + 1; // avoid 0 - that's for TID NULL
	// lab3
	task_blocks[space].deadline = deadline; // Default deadline 5ms
	task_blocks[space].time_remaining = deadline;
	task_blocks[space].sleep_time = 0;

	tcb_in->tid = task_blocks[space].tid;
	tasks_available++;
	

/*-Changing approach: global_stackptr -> mem alloc handles placement (Need to discuss)-*/

	// task_blocks[space].stack_high = (U32)global_stackptr;
	// global_stackptr = (U32*)((char*)global_stackptr - tcb_in->stack_size);
	task_blocks[space].stack_high =  (U32)((U8*)stack_mem + tcb_in->stack_size);
/*-------------------------------------------------------------------------------------*/

	U32 * cur_stackptr = (U32*)task_blocks[space].stack_high;
	/* enable THUMB mode for the task */
	*(--cur_stackptr) = 1 << 24; 			  // xPSR
	/* stores address of c */
	*(--cur_stackptr) = (U32)task_blocks[space].ptask; // PC
	 // LR
	/* set dummy values to R4-R11*/
	for (int i = 0;  i < 14; i++) {
		*(--cur_stackptr) = 0xA;
	}
	//setting the PSP of the stack
    task_blocks[space].psp_stack_ptr = (U32*)cur_stackptr;

	/* lab3 */
    // Pre-emption: if new task's deadline is sooner than current, trigger context switch
    if (task_current && (task_blocks[space].time_remaining < task_current->time_remaining)) {
        task_current->state = READY;
        task_blocks[space].state = RUNNING;
        task_current = &task_blocks[space];
        // Trigger PendSV for context switch
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        __asm__ volatile("isb");
        __asm__ volatile("dsb");
    }

	return RTX_OK;
}

void initializeKernel(void) {
	for(int i = 0; i < MAX_TASKS; i++){
		task_blocks[i].ptask = RTX_NULL;
		task_blocks[i].stack_high = 0;
		task_blocks[i].psp_stack_ptr = 0;
		task_blocks[i].state = DORMANT;
		task_blocks[i].tid = UNINT_TASK_ID;				// unintialized state
		task_blocks[i].stack_size = STACK_SIZE;

		//task 3
		task_blocks[i].deadline = 0;
		task_blocks[i].time_remaining = 0;
		task_blocks[i].sleep_time = 0;

	}
	// Setup null task
	tasks_available = 0;
	state = KERNEL_INIT;

	//set up null task
	task_blocks[TID_NULL].ptask = &null_task;
	task_blocks[TID_NULL].state = READY;
	task_blocks[TID_NULL].stack_size = 0x200; //temp and can be modified
	task_blocks[TID_NULL].tid = TID_NULL;
	// set deadline and time
	task_blocks[TID_NULL].deadline = 1000;
	task_blocks[TID_NULL].time_remaining = 1000;
	task_blocks[TID_NULL].sleep_time = 0;
}

int findNextReadyTask(void) {
    int min_idx = 0;
    // Search for the next READY task, provided it is READY and not SLEEPING
    for (int i = 0; i < MAX_TASKS; i++) { // <= to ensure full wrap-around
    	if (task_blocks[i].tid != UNINT_TASK_ID && task_blocks[i].state == READY && task_blocks[i].tid != task_current->tid) {

    		if (task_blocks[i].time_remaining < task_blocks[min_idx].time_remaining) {
            	min_idx = i;
            	//break tie by choosing task with lower TID
        	} else if (task_blocks[i].time_remaining == task_blocks[min_idx].time_remaining) {
        		if (task_blocks[i].tid < task_blocks[min_idx].tid)
        			min_idx = i;

        	}
        }
    }


    return min_idx;
}

void scheduler(void) {
    int next = findNextReadyTask();
    if (next != -1) {
        task_current = &task_blocks[next];
        task_current->state = RUNNING;
    }
}

int startKernel(void){
	// validation
	if(state == KERNEL_RUN || state == KERNEL_UNINIT || tasks_available == 0){
		return RTX_ERR;
	}


	TCB* task1 = &task_blocks[findNextReadyTask()];

	state = KERNEL_RUN;
	task_current = task1;
	task1->state = RUNNING;

	SysTick->VAL = 0;

    __asm__ volatile("svc %[immediate]" :: [immediate] "I"(SVC_LOAD_PSP));

	return RTX_ERR; // should not reach this
}

__attribute__((naked)) void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */
	__asm__ volatile (
	    ".global SVC_Handler_Main\n"
	    "TST lr, #4\n"
	    "ITE EQ\n"
	    "MRSEQ r0, MSP\n"
	    "MRSNE r0, PSP\n"
	    "B SVC_Handler_Main\n"
	) ;

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */
  /* USER CODE END SVCall_IRQn 1 */
}

void SVC_Handler_Main(unsigned int *svc_args)
{
	unsigned int svc_number;
	svc_number = ((char *)svc_args[6])[-2];

	/* SVC #0 is reserved */
	/* handles OS functions based on SVC call */
	  switch( svc_number ) {

	    case SVC_LOAD_PSP:  /* switch to handler mode to set up thread stack pointer */
	      load_psp(task_current->psp_stack_ptr);
	      break;

		case SVC_CREATE_TASK:
				TCB* tcb_input = (TCB*)svc_args[0]; // Not sure if this is zero
				int res = createTCB(tcb_input, 5);
				svc_args[0] = res; //store return value

			break;

		case SVC_YIELD:
			if (task_current != RTX_NULL && state == KERNEL_RUN) {
				// Only set to READY if not already SLEEPING
				if (task_current->state != SLEEPING) {
					task_current->state = READY;
				}
				// Trigger PendSV for context switch
				SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

				//synchronize the registers and make the interrupts happen
				__asm__ volatile("isb");
				__asm__ volatile("dsb");
			}
			break;

		case SVC_TASK_INFO:
			task_t tid = (task_t)svc_args[0];
			TCB* task_copy = (TCB*)svc_args[1];

			// Find task with given TID
			int found = 0;
			for(int i = 0; i < MAX_TASKS; i++) {
				if(task_blocks[i].tid == tid && task_blocks[i].state != DORMANT) {
					// Copy TCB data
					*task_copy = task_blocks[i];
					found = 1;
					break;
				}
			}
			svc_args[0] = found ? RTX_OK : RTX_ERR;

			break;

		case SVC_GET_TID:
			if(state == KERNEL_RUN && task_current != RTX_NULL) svc_args[0] = task_current->tid;
			else svc_args[0] = 0;  // TID_NULL whenA kernel not started or no current task
			break;

		case SVC_TASK_EXIT:
			if (task_current != RTX_NULL && state == KERNEL_RUN){
				mem_blk_t* block = ((mem_blk_t*)((void*)(task_current->stack_high - task_current->stack_size))) - 1;
				if(block->magic == MAGIC_NUMBER){
					block->owner_TID = task_current->tid;
				}
				k_mem_dealloc((void*)(task_current->stack_high - task_current->stack_size)); //getting lowest addr
				task_current->state = DORMANT;
				task_current->psp_stack_ptr = 0;
				task_current->stack_high = 0;
				task_current->stack_size = 0;
				task_current->deadline = 0;
				task_current->sleep_time = 0;
				task_current->time_remaining = 0;
	            tasks_available--;


				// Trigger PendSV for context switch
				SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
				__asm__ volatile("isb");
				__asm__ volatile("dsb");
			} else {
				svc_args[0] = RTX_ERR;
			}
			break;

		case SVC_CREATE_DEAD_TASK:
			int deadline1 = svc_args[0];
			TCB* tcb_input1 = (TCB*)svc_args[1]; // Not sure if this is zero
			int res1 = createTCB(tcb_input1, deadline1);
			svc_args[0] = res1; //store return value
			break;

		default:
	      break;
	  }
}

void osKernelInit(void){
	// global_stackptr = MSP_INIT_VAL - MSP_STACK_SIZE; //move stackptr to first thread stack
	// global_start_ptr = MSP_INIT_VAL - MSP_STACK_SIZE;
	//changing priorities of interrupts
	SHPR3 = (SHPR3 & ~(0xFFU << 24)) | (0xF0U << 24);//SysTick is lowest priority (highest number)
	SHPR3 = (SHPR3 & ~(0xFFU << 16)) | (0xE0U << 16);//PendSV is in the middle
	SHPR2 = (SHPR2 & ~(0xFFU << 24)) | (0xD0U << 24);//SVC is highest priority (lowest number)
	initializeKernel();
}

int osCreateTask(TCB* task){
	// check ptr
    if(task == RTX_NULL || task->stack_size < STACK_SIZE || task->stack_size > STACK_SIZE*MAX_TASKS){
        return RTX_ERR;
    }

    int result;
    __asm__ volatile(
        "mov r0, %1\n"          // Pass TCB pointer as first argument
        "svc %2\n"              // Make SVC call
        "mov %0, r0\n"          // Get return value
        : "=r" (result)         // Output: result variable
        : "r" (task), "I" (SVC_CREATE_TASK)  // Inputs: task pointer and SVC number
        : "r0"                  // Clobbered registers
    );
    
    return result;
}

int osKernelStart(void){
	/* start running a kernel thread */

	startKernel();
	return RTX_ERR;
}

void osYield(void){
    if (task_current) {
        task_current->time_remaining = task_current->deadline;
    }
    __asm__ volatile("svc %0" :: "I" (SVC_YIELD));
}

void osSleep(int timeInMs) {
	if (task_current) {
		task_current->sleep_time = timeInMs;
		task_current->state = SLEEPING;
		osYield();
	}
}

void osPeriodYield() {
	//is the period here correct? to the remainder of the period?
	if (task_current) {
		task_current->sleep_time = task_current->time_remaining;
		osSleep(task_current->sleep_time);
	}

}

int osTaskInfo(task_t TID, TCB* task_copy){
    if(task_copy == RTX_NULL) return RTX_ERR;
    
    int result;
    __asm__ volatile(
        "mov r0, %1\n"
        "mov r1, %2\n"
        "svc %3\n"
        "mov %0, r0\n"
        : "=r" (result)
        : "r" (TID), "r" (task_copy), "I" (SVC_TASK_INFO)
        : "r0", "r1"
    );
    return result;
}

task_t osGetTID(void){
    task_t result;
    __asm__ volatile(
        "svc %1\n"
        "mov %0, r0\n"
        : "=r" (result)
        : "I" (SVC_GET_TID)
        : "r0"
    );
    return result;
}

int osTaskExit(void){
	int result;
	__asm__ volatile(
		"svc %1\n"				// Trigger SVC exception with SVC_TASK_EXIT (63)
		"mov %0, r0\n"			// Move the value from register 0 to the C variable 'result'
		: "=r" (result)			// Output: result variable
		: "I" (SVC_TASK_EXIT)	// Input: SVC_TASK_EXIT number (63)
		: "r0"
	);
	return result;
}

int osCreateDeadlineTask(int deadline, TCB* task){
	if(task == RTX_NULL || task->stack_size < STACK_SIZE || task->stack_size > STACK_SIZE*MAX_TASKS || deadline <=0){
        return RTX_ERR;
    }
	int result;
    __asm__ volatile(
        "mov r0, %1\n"          // Pass deadline as first argument
        "mov r1, %2\n"          // Pass TCB pointer as second argument
        "svc %3\n"              // Make SVC call
        "mov %0, r0\n"          // Get return value
        : "=r" (result)         // Output: result variable
        : "r" (deadline), "r" (task), "I" (SVC_CREATE_DEAD_TASK)  // Inputs: deadline, task pointer and SVC number
        : "r0", "r1"            // Clobbered registers
    );
    
    return result;
}

int osSetDeadline(int deadline, task_t TID){
	if (deadline <= 0 || TID == osGetTID()) {
        return RTX_ERR;
    }
	__disable_irq();
	int found = 0;
	for (int i = 0; i < MAX_TASKS; i++) {
		if (task_blocks[i].tid == TID && task_blocks[i].state == READY) {
			task_blocks[i].deadline = deadline;
			found = i; // task found and deadline set
			break;
		}
	}
	__enable_irq();
	// Still at NULL task.
	if (found == 0) return RTX_ERR;


	if (task_current && task_blocks[found].deadline < task_current->deadline)
        osYield();

    return RTX_OK;
}

