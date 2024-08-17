[BITS 32]

section .asm

global restore_general_purpose_registers
global task_return
global user_registers

; void task_return(struct registers* regs);
task_return:
    mov ebp, esp                ; Set the base pointer (ebp) to the current stack pointer (esp).

    mov ebx, [ebp + 4]          ; Load the pointer to the 'regs' structure into ebx.

    push dword [ebx + 44]       ; Push 'regs->cs' (Code Segment) onto the stack.
    push dword [ebx + 40]       ; Push 'regs->ip' (Instruction Pointer) onto the stack.

    pushf                       ; Push the current flags register (EFLAGS) onto the stack.
    pop eax                     ; Pop the flags into eax to modify them.
    or eax, 0x200               ; Set the interrupt flag (IF) in the EFLAGS register to enable interrupts.
    push eax                    ; Push the modified EFLAGS back onto the stack.

    push dword [ebx + 32]       ; Push 'regs->ss' (Stack Segment) onto the stack.

    push dword [ebx + 28]       ; Push 'regs->esp' (Stack Pointer) onto the stack.

    mov ax, [ebx + 44]          ; Load 'regs->cs' (Code Segment) into ax.
    mov ds, ax                  ; Set the data segment (DS) register to the value in ax (same as CS).
    mov es, ax                  ; Set the extra segment (ES) register to the value in ax (same as CS).
    mov fs, ax                  ; Set the FS segment register to the value in ax (same as CS).
    mov gs, ax                  ; Set the GS segment register to the value in ax (same as CS).

    push dword [ebx + 4]        ; Push the pointer to the general-purpose registers in 'regs' (starting with edi) onto the stack.
    call restore_general_purpose_registers ; Call the function to restore general-purpose registers.

    add esp, 4                  ; Clean up the stack by removing the pushed argument for restore_general_purpose_registers.


    ; Let's leave kernel land and execute in user land!
    iretd                       ; Interrupt return: pops CS, IP, and EFLAGS from the stack and resumes execution.




; void restore_general_purpose_registers(struct registers* regs);
restore_general_purpose_registers:
    push ebp             ; Save the current base pointer value on the stack.
    mov ebp, esp         ; Set the base pointer (ebp) to the current stack pointer (esp).
    mov ebx, [ebp + 8]   ; Load the first argument (a pointer to the saved registers) into ebx.
    
    mov edi, [ebx]       ; Restore the value of the edi register from the saved state.
    mov esi, [ebx + 4]   ; Restore the value of the esi register from the saved state.
    mov ebp, [ebx + 8]   ; Restore the value of the ebp register from the saved state.
    mov edx, [ebx + 16]  ; Restore the value of the edx register from the saved state.
    mov ecx, [ebx + 20]  ; Restore the value of the ecx register from the saved state.
    mov eax, [ebx + 24]  ; Restore the value of the eax register from the saved state.
    mov ebx, [ebx + 12]  ; Restore the value of the ebx register from the saved state.

    pop ebp              ; Restore the old base pointer value from the stack.
    ret                  ; Return from the function, restoring the instruction pointer to the caller.


; void user_registers();
user_registers:
    mov ax, 0x23                ; Load the user data segment selector (0x23) into ax.
    mov ds, ax                  ; Set the data segment register (ds) to user mode.
    mov es, ax                  ; Set the extra segment register (es) to user mode.
    mov fs, ax                  ; Set the FS segment register to user mode.
    mov gs, ax                  ; Set the GS segment register to user mode.
    ret
    
