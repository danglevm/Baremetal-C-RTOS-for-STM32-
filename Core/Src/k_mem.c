#include "k_mem.h"

// Addresses in RAM, used for Heap Boundaries
extern U8 _img_end;        // End of Static Image in RAM (Start of Heap)
extern U8 _estack;         // Top of Stack Mem (Top of RAM)
extern U32 _Min_Stack_Size; // Size of stack to preserve
static U8* heap_start_addr = RTX_NULL;  // Start address of heap
static U8* heap_end_addr = RTX_NULL;    // End address of heap
static size_t heap_size_total = 0;      // Total heap size

extern kernel_state state;
extern TCB* task_current;

static int mem_initialized = 0;         // 0: not initialized, 1: initialized
static mem_blk_t* freelist_head = RTX_NULL; // head of free list

int k_mem_init(void) {
   if (mem_initialized || state == KERNEL_UNINIT) {
       return RTX_ERR;                                     // Memory already initialized
   }
   /* ----Head Boundaries---- */
   U8* heap_start = (U8*)&_img_end + 0x200;                // Optional Offset
   U8* heap_end = (U8*)((U32)&_estack - (U32)&_Min_Stack_Size);

   // Align heap_start and heap_end to 4-byte boundary
   heap_start = (U8*) ((U32)heap_start + 3);                 // Go over any unaligned address
   heap_start = (U8*)((U32)heap_start & ~0x3);             // Clear last 2 bits to align to 4-byte boundary

   heap_end = (U8*)((U32)heap_end & ~0x3);                 // Clear last 2 bits to align to 4-byte boundary

   if (heap_start >= heap_end) {
       return RTX_ERR;                                     // No Space for Heap
   }

   // Store heap boundaries for later use
   heap_start_addr = heap_start;
   heap_end_addr = heap_end;
   heap_size_total = heap_end - heap_start;

   // Initialize with empty free list
   freelist_head = RTX_NULL;

   mem_initialized = 1;                                    // Mark memory as initialized
   return RTX_OK;
}

void* k_mem_alloc(size_t size){
    if (!mem_initialized || size == 0) {
        return RTX_NULL;
    }
    size = (size + 3) & ~0x3;        // Align size to 4-byte boundary

    // If heap is empty, create the first free block
    if (freelist_head == RTX_NULL) {
        freelist_head = (mem_blk_t*)heap_start_addr;
        freelist_head->size = heap_size_total - sizeof(mem_blk_t);
        freelist_head->is_free = 1;
        freelist_head->owner_TID = UNINT_TASK_ID;
        freelist_head->next = RTX_NULL;
        freelist_head->magic = MAGIC_NUMBER;
    }
    
    mem_blk_t* prev = RTX_NULL;      // Previous block pointer
    mem_blk_t* curr = freelist_head; // Current for linkedlist traversal

    size_t total_size = size + sizeof(mem_blk_t);
    while (curr != RTX_NULL){
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= total_size + 4) { // Check if block is big enough to split
                mem_blk_t* new_blk = (mem_blk_t*)((U8*)(curr + 1) + size); // Create a new block at location after metadata + size
                new_blk->size = curr->size - total_size;
                new_blk->is_free = 1;
                new_blk->owner_TID = UNINT_TASK_ID;
                new_blk->next = curr->next;
                new_blk->magic = MAGIC_NUMBER;

                curr->size = size;                  // Updating size to the size requested
                curr->next = new_blk;               // Used for traversing to the new block
            }
            curr->is_free = 0;                                                 // Marked as allocated
            curr->owner_TID = (task_current ? task_current->tid : KERNEL_TID); // If there is a running task, it becomes the owner, else kernel takes ownership
            
            /* Skip curr to remove curr from free list */
            if (prev == RTX_NULL) {
                freelist_head = curr->next; 
            } else {
                prev->next = curr->next;
            }

            return (void*)(curr + 1); // Return a pointer to allocated memory, which is after metadata
        }
        prev = curr;
        curr = curr->next;
    }
    return RTX_NULL; // No space found
}

int k_mem_dealloc(void* ptr){
    if(ptr == RTX_NULL){
        return RTX_OK;
    }
    if(!mem_initialized){
        return RTX_ERR;
    }

    // get metadata block
    mem_blk_t* block = ((mem_blk_t*)ptr) - 1; // moves us back by 1 mem_blk_t structure

    // check if valid metadata (ptr in middle of some memory - bad)
    if(block->magic != MAGIC_NUMBER){
        return RTX_ERR;
    }

    // check the block is allocated and dealloc request is from owner
    task_t current_tid = (task_current ? task_current->tid : KERNEL_TID);
    if(block->is_free == 1 || block->owner_TID != current_tid){
        return RTX_ERR;
    }

    // now we can deallocate safely
    block->is_free = 1;
    block->owner_TID = UNINT_TASK_ID;

    // find correct place in free list
    mem_blk_t* prev = RTX_NULL;
    mem_blk_t* curr = freelist_head;
    while (curr != RTX_NULL && (U8*)curr < (U8*)block) {
        prev = curr;
        curr = curr->next;
    }
    // place it in
    if(prev == RTX_NULL){
        block->next = freelist_head;
        freelist_head = block;
    }
    else{
        prev->next = block;
        block->next = curr;
    }
    curr = block;
    
    // coalesce with previous block
    if (prev != RTX_NULL && prev->is_free == 1) {
        // sanity check (should not be required if free list is made properly)
        mem_blk_t* prev_end = (mem_blk_t*)((U8*)(prev + 1) + prev->size);  
        if (prev_end == block) {
            prev->size += sizeof(mem_blk_t) + block->size;
            prev->next = block->next;
            curr = prev;
        }
    }

    // coalesce with next block
    if(curr->next != RTX_NULL && curr->next->is_free == 1){
        // sanity check
        mem_blk_t* curr_end = (mem_blk_t*)((U8*)(curr + 1) + curr->size);
        if(curr_end == curr->next){
            curr->size += sizeof(mem_blk_t) + curr->next->size;
            curr->next = curr->next->next;
        }
    }
   // Check if this is now the only free block and it covers the entire heap
    if (freelist_head != RTX_NULL && freelist_head->next == RTX_NULL) {
        // Check if this single block covers the entire heap
        if (freelist_head->size + sizeof(mem_blk_t) >= heap_size_total) {
            // Clear the heap completely
            freelist_head = RTX_NULL;
        }
    }
    return RTX_OK;
}

int k_mem_count_extfrag(size_t size){
    if(mem_initialized != 1){
        return 0;
    }
    mem_blk_t* temp = freelist_head;
    int count = 0;
    while(temp != RTX_NULL){
        if(temp->size + sizeof(mem_blk_t) < size){
            count++;
        }
        temp = temp->next;
    }
    return count;
}

