.thumb
.syntax unified

.global PendSV_Handler

.global load_psp


.type PendSV_Handler, %function

.type load_psp, %function

PendSV_Handler:
		MRS R0, PSP
		STMDB R0!, {R4-R11}
	    /* Save updated stack pointer to current TCB */
	    LDR R1, =task_current
	    LDR R2, [R1]
        STR R0, [R2, #0]
        /* Call scheduler() */
        BL scheduler
        LDR   R1, =task_current
   		LDR   R2, [R1]         // Load the NEW task_current pointer, which scheduler updated.
    	// Load the next task's stack pointer from tcb->psp_stack_ptr (Offset 0).
    	LDR   R0, [R2, #0]

    	LDR LR, =0xFFFFFFFD //exit handler mode and return to thread mode
        /* Restore registers for next task */
        //load starting from PSP, which causes it to crash
        LDMIA R0!, {R4-R11}
        MSR PSP, R0
        BX LR


load_psp:
	    LDMIA R0!, {R4-R11}
		MSR PSP, R0			 //initialize stack pointer to starting address stored in stackptr
		LDR LR, =0xFFFFFFFD //exit handler mode and return to thread mode
	    BX LR
