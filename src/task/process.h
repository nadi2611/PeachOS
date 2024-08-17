#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include "kernel.h"
#include "task.h"
#include "config.h"

struct process
{
    uint16_t id;                     // Unique identifier for the process.

    char filename[PEACHOS_MAX_PATH]; // The name of the file associated with the process, usually the executable's path.

    struct task* task;               // Pointer to the task associated with this process. This represents the execution context of the process.

    void* allocations[PEACHOS_MAX_PROGRAM_ALLOCATIONS]; 
                                      // Array of pointers to memory allocations made by the process.
                                      // These could be used for dynamic memory management, such as heap allocations.

    void* ptr;                        // General-purpose pointer, could be used to reference the process's memory or data structure.

    void* stack;                      // Pointer to the process's stack in memory, where local variables and return addresses are stored during execution.

    uint32_t size;                    // The size of the process in memory, typically representing the total memory footprint of the process.
};


int process_load_for_slot(const char* filename, struct process** process, int process_slot);

#endif