#include "task.h"
#include "memory/memory.h"
#include "kernel.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "process.h"

// Pointer to the currently running task
struct task* current_task = 0;

// Pointer to the last task in the task list (tail of the linked list)
struct task* task_tail = 0;

// Pointer to the first task in the task list (head of the linked list)
struct task* task_head = 0;

/**
 * Function to get the currently running task.
 *
 * @return Pointer to the current task.
 */
struct task* task_current()
{
    return current_task;
}

/**
 * Creates and initializes a new task.
 *
 * Allocates memory for a new task, initializes it, and adds it to the task list.
 * @param process Pointer to the process structure to be initialized.
 * @return Pointer to the newly created task, or an error code on failure.
 */
struct task* task_new(struct process* process)
{
    int res = 0;
    struct task* task = kzalloc(sizeof(struct task)); // Allocate memory for a new task
    if(!task)
    {
        res = -ENOMEM; // Return error if memory allocation fails
        goto out;
    }

    res = task_init(task, process); // Initialize the new task
    if(res != PEACHOS_ALL_OK)
    {
        goto out; // If initialization fails, jump to cleanup
    }

    if(task_head == 0)
    {
        task_head = task; // Set as the head of the list if it's the first task
        task_tail = task; // Set as the tail of the list
        goto out;
    }

    task_tail->next = task; // Add the new task to the end of the task list
    task->prev = task_tail; // Set the previous task as the current tail
    task_tail = task;       // Update the tail to be the new task
    
out:
    if(ISERR(res))
    {
        task_free(task); // Free the task if there was an error
        return ERROR(res);
    }
    return task; // Return the new task
}

/**
 * Retrieves the next task in the task list.
 *
 * @return Pointer to the next task, or the head of the list if there is no next task.
 */
struct task* task_get_next()
{
    if(!current_task->next)
    {
        return task_head; // If there is no next task, return the head of the list
    }

    return current_task->next; // Return the next task in the list
}

/**
 * Removes a task from the task list.
 *
 * This function adjusts the pointers in the linked list to exclude the specified task.
 *
 * @param task Pointer to the task to be removed.
 */
static void task_list_remove(struct task* task)
{
    if(task->prev)
    {
        task->prev->next = task->next; // Adjust the previous task's next pointer
    }
    if(task == task_head)
    {
        task_head = task->next; // Update the head if the task was the head
    }

    if(task == task_tail)
    {
        task_tail = task->prev; // Update the tail if the task was the tail
    }

    if (task == current_task)
    {
        current_task = task_get_next(); // Move to the next task if the current task was removed
    }
}

/**
 * Frees the resources associated with a task.
 *
 * This function releases the paging directory and memory associated with the given task,
 * and removes it from the task list.
 *
 * @param task Pointer to the task to be freed.
 * @return 0 on success.
 */
int task_free(struct task* task)
{
    paging_free_4gb(task->page_directory); // Free the 4GB paging directory
    task_list_remove(task);                // Remove the task from the list

    kfree(task); // Free the memory allocated for the task structure
    return 0;    // Return success
}

/**
 * @brief Switches the current task to a new task.
 *
 * This function updates the global `current_task` to point to the new task provided as an argument.
 * It also switches the memory context by switching the page directory to that of the new task.
 *
 * @param task A pointer to the task structure representing the new task to switch to.
 * @return Always returns 0 to indicate success.
 */
int task_switch(struct task* task)
{
    // Set the global variable `current_task` to the task that is being switched to.
    current_task = task;
    
    // Switch the current paging directory to the one associated with the new task.
    // This changes the memory context to the new task's virtual memory space.
    paging_switch(task->page_directory->directory_entry);
    
    // Return 0 to indicate the task switch was successful.
    return 0;
}

/**
 * @brief Prepares the task environment and switches to the current task.
 *
 * This function first sets up the user mode registers by calling `user_registers`.
 * It then switches to the current task using `task_switch`, which prepares the
 * memory and context for the task.
 *
 * @return Always returns 0 to indicate success.
 */
int task_page()
{
    // Set up the user mode registers and prepare for user mode execution.
    user_registers();
    
    // Switch to the current task. This will set up the memory and context for the task.
    task_switch(current_task);
    
    // Return 0 to indicate that the operation was successful.
    return 0;
}

/**
 * @brief Runs the first task ever scheduled in the system.
 *
 * This function is responsible for switching to and starting the first task in the system.
 * It ensures that there is a valid `current_task` and then switches to the first task (`task_head`).
 * The function then returns to the execution context of the first task by restoring its registers and state.
 * If no `current_task` is set, the function will trigger a panic.
 */
void task_run_first_ever_task()
{
    // If there is no current task set, trigger a panic because the system cannot continue.
    if (!current_task)
    {
        panic("task_run_first_ever_task(): No current task exists!");
    }

    // Switch to the first task in the task list (`task_head`).
    task_switch(task_head);
    
    // Return to the execution context of the first task by restoring its registers and state.
    // This resumes execution from where the first task left off or starts it if it hasn't run yet.
    task_return(&task_head->registers);
}


/**
 * Initializes a new task structure.
 *
 * @param task Pointer to the task structure to be initialized.
 * @param process Pointer to the process structure to be initialized.
 * @return 0 on success, or an error code (e.g., -EIO) on failure.
 */
int task_init(struct task* task, struct process* process)
{
    // Clear the memory for the task structure
    memset(task, 0x00, sizeof(task));

    // Create a new 4GB paging directory for the task with necessary flags
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if(!task->page_directory)
    {
        return -EIO; // Return an error code if the page directory creation fails
    }

    // Set up initial CPU register values for the task
    task->registers.ip = PEACHOS_PROGRAM_VIRTUAL_ADDRESS; // Initial instruction pointer (IP)
    task->registers.ss = USER_DATA_SEGMENT;               // Stack segment (SS)
    task->registers.esp = PEACHOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START; // Initial stack pointer (ESP)

    task->process = process;                             // Initial process

    return 0; // Return 0 on successful task initialization
}
