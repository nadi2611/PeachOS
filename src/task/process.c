#include "process.h"            // Include the header file that defines the process-related functions and structures.
#include "memory/memory.h"      // Include the memory management functions.
#include "status.h"             // Include status code definitions.
#include "task/task.h"          // Include task-related functions and structures.
#include "memory/heap/kheap.h"  // Include heap memory management functions.
#include "fs/file.h"            // Include file system functions.
#include "string/string.h"      // Include string manipulation functions.
#include "memory/paging/paging.h" // Include paging functions for memory management.

// Pointer to the currently active process.
struct process* current_process = 0;

// Array to hold pointers to all processes. Initialized to null (0) for each slot.
static struct process* processes[PEACHOS_MAX_PROCESSES] = {};

/**
 * Initializes the given process structure by setting all its fields to zero.
 *
 * @param process Pointer to the process structure to be initialized.
 */
static void process_init(struct process* process)
{
    memset(process, 0x00, sizeof(struct process)); // Clear the process structure.
}

/**
 * Returns a pointer to the currently active process.
 *
 * @return Pointer to the current process.
 */
struct process* process_current()
{
    return current_process;
}

/**
 * Retrieves the process structure for a given process ID.
 *
 * @param process_id The ID of the process to retrieve.
 * @return Pointer to the process structure, or an error code if the ID is invalid.
 */
struct process* process_get(int process_id)
{
    if (process_id < 0 || process_id >= PEACHOS_MAX_PROCESSES)
    {
        return NULL; // Return an error if the process ID is out of range.
    }

    return processes[process_id]; // Return the process pointer from the array.
}

/**
 * Loads a binary file into the process's memory.
 *
 * @param filename The name of the binary file to load.
 * @param process Pointer to the process structure.
 * @return 0 on success, or an error code on failure.
 */
static int process_load_binary(const char* filename, struct process* process)
{
    int res = 0;
    int fd = fopen(filename, "r"); // Open the file for reading.
    if (!fd)
    {
        res = -EIO; // Return an error if the file couldn't be opened.
        goto out;
    }

    struct file_stat stat;

    res = fstat(fd, &stat); // Get the file's size and other metadata.
    if(res != PEACHOS_ALL_OK)
    {
        goto out; // Return an error if stat retrieval failed.
    }

    void* program_data_ptr = kzalloc(stat.file_size); // Allocate memory for the file's content.
    if(!program_data_ptr)
    {
        res = -ENOMEM; // Return an error if memory allocation failed.
        goto out;
    }

    if(fread(program_data_ptr, stat.file_size, 1, fd) != 1)
    {
        res = -EIO; // Return an error if file reading failed.
        goto out;
    }

    process->ptr = program_data_ptr; // Store the pointer to the loaded data in the process structure.
    process->size = stat.file_size; // Store the size of the loaded data.
    
out:  
    fclose(fd); // Close the file.
    return res; // Return the result of the operation.
}

/**
 * Loads the binary data for the given process.
 *
 * @param filename The name of the binary file to load.
 * @param process Pointer to the process structure.
 * @return 0 on success, or an error code on failure.
 */
static int process_load_data(const char* filename, struct process* process)
{
    int res = 0;
    res = process_load_binary(filename, process); // Load the binary data for the process.
    return res;
}

/**
 * Maps the process's binary data into its virtual address space.
 *
 * @param process Pointer to the process structure.
 * @return 0 on success, or an error code on failure.
 */
int process_map_binary(struct process* process)
{
    int res = 0;
    paging_map_to(
        process->task->page_directory->directory_entry, 
        (void*)PEACHOS_PROGRAM_VIRTUAL_ADDRESS, 
        process->ptr, 
        paging_align_address(process->ptr + process->size), 
        PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE
    ); // Map the binary data into the process's address space.

    return res; // Return the result of the operation.
}

/**
 * Maps the memory regions required by the process, including its binary data.
 *
 * @param process Pointer to the process structure.
 * @return 0 on success, or an error code on failure.
 */
int process_map_memory(struct process* process)
{
    int res = 0;
    res = process_map_binary(process); // Map the binary data into the process's address space.
    return res;
}

/**
 * Finds a free slot in the process table.
 *
 * @return The index of the first free slot if found, otherwise returns -EISTKN indicating that no free slot is available.
 */
int process_get_free_slot()
{
    // Iterate through all possible process slots.
    for (int i = 0; i < PEACHOS_MAX_PROCESSES; i++)
    {
        // If the current slot is empty (i.e., no process is assigned to it), return this slot index.
        if (processes[i] == 0)
        {
            return i;
        }
    }

    // If no free slot is found, return an error code indicating that all slots are taken.
    return -EISTKN;
}

/**
 * Loads a process from a binary file and assigns it to a free slot.
 *
 * @param filename The name of the file containing the process to be loaded.
 * @param process A pointer to the process structure that will be allocated and loaded.
 * @return 0 on success, or a negative error code on failure.
 */
int process_load(const char* filename, struct process** process)
{
    int res = 0;

    // Get the first available slot for the new process.
    int process_slot = process_get_free_slot();
    
    // If no free slot is available, return an error code indicating memory allocation failure.
    if (process_slot < 0)
    {
        res = -ENOMEM;
        goto out;
    }

    // Load the process into the allocated slot.
    res = process_load_for_slot(filename, process, process_slot);

out:
    // Return the result, which could be 0 for success or an error code for failure.
    return res;
}


/**
 * Loads a process into a specific slot in the process array.
 *
 * @param filename The name of the binary file to load.
 * @param process Pointer to store the created process.
 * @param process_slot The slot in the process array to load the process into.
 * @return 0 on success, or an error code on failure.
 */
int process_load_for_slot(const char* filename, struct process** process, int process_slot)
{
    int res = 0;
    struct task* task = 0;
    struct process* _process;
    void* program_stack_ptr = 0;

    if(process_get(process_slot) != 0)
    {
        res = -EISTKN; // Return an error if the slot is already occupied.
        goto out;
    }

    _process = kzalloc(sizeof(struct process)); // Allocate memory for the process structure.
    if(!_process)
    {
        res = -ENOMEM; // Return an error if memory allocation failed.
        goto out;
    }

    process_init(_process); // Initialize the process structure.
    res = process_load_data(filename, _process); // Load the binary data for the process.
    if(res < 0)
    {
        goto out;
    }

    program_stack_ptr = kzalloc(PEACHOS_USER_PROGRAM_STACK_SIZE); // Allocate memory for the process's stack.
    if(!program_stack_ptr)
    {
        res = -ENOMEM; // Return an error if memory allocation failed.
        goto out;
    }

    strncpy(_process->filename, filename, sizeof(_process->filename)); // Copy the filename into the process structure.
    _process->stack = program_stack_ptr; // Store the pointer to the stack in the process structure.
    _process->id = process_slot; // Set the process ID to the slot number.

    task = task_new(_process); // Create a new task for the process.
    if(ERROR_I(task) == 0)
    {
        res = ERROR_I(task); // Return an error if task creation failed.
    }

    _process->task = task; // Store the task pointer in the process structure.

    res = process_map_memory(_process); // Map the process's memory regions.
    if(res < 0)
    {
        goto out;
    }

    *process = _process; // Store the process pointer in the output parameter.

    process[process_slot] = _process; // Store the process in the global process array.

out:
    if(ISERR(res))
    {
        if(_process && _process->task)
        {
            task_free(_process->task); // Free the task if an error occurred.
        }

        // Free process data (e.g., stack, binary data) if needed.
    }
    return res; // Return the result of the operation.
}
