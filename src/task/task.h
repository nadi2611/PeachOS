#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"


struct registers {
    uint32_t edi;   // Destination Index register (EDI). Used in string operations.
    uint32_t esi;   // Source Index register (ESI). Used in string operations.
    uint32_t ebp;   // Base Pointer register (EBP). Points to the base of the current stack frame.
    uint32_t ebx;   // Base register (EBX). General-purpose register often used to store data.
    uint32_t edx;   // Data register (EDX). General-purpose register often used in I/O operations.
    uint32_t ecx;   // Counter register (ECX). Used for loop counting and shift operations.
    uint32_t eax;   // Accumulator register (EAX). Used for arithmetic operations and as a return value from functions.

    uint32_t ip;    // Instruction Pointer (IP). Holds the address of the next instruction to execute.
    uint32_t cs;    // Code Segment (CS). Segment selector for the code segment.
    uint32_t flags; // Flags register. Stores the current state of the processor flags (condition codes, control bits).
    uint32_t esp;   // Stack Pointer (ESP). Points to the top of the current stack.
    uint32_t ss;    // Stack Segment (SS). Segment selector for the stack segment.
};

struct process;
struct task {
    struct paging_4gb_chunk* page_directory; // Pointer to the page directory for the task, defining the task's virtual memory space.
    struct registers registers;             // Pointer to the CPU registers state for the task, used to save and restore the task's context.
    struct process* process;                // Pointer to the process of the task.
    struct task* next;                       // Pointer to the next task in the task list, forming a linked list of tasks.
    struct task* prev;                       // Pointer to the previous task in the task list, allowing traversal in both directions.
};


int task_init(struct task* task, struct process* process);
int task_free(struct task* task);
struct task* task_get_next();
struct task* task_new(struct process* process);
struct task* task_current();
int task_switch(struct task* task);
int task_page();

void task_run_first_ever_task();

void task_return(struct registers* regs);
void restore_general_purpose_registers(struct registers* regs);
void user_registers();

#endif