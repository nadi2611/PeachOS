#ifndef TSS_H
#define TSS_H

#include <stdint.h>

struct tss {
    uint32_t link;    // Previous Task State Segment (TSS) selector. Used for task switching.
    uint32_t esp0;    // Kernel stack pointer (ring 0). Loaded into `ESP` when switching to ring 0.
    uint32_t ss0;     // Kernel stack segment (ring 0). Loaded into `SS` when switching to ring 0.
    uint32_t esp1;    // Stack pointer for ring 1 (unused in most systems).
    uint32_t ss1;     // Stack segment for ring 1 (unused in most systems).
    uint32_t esp2;    // Stack pointer for ring 2 (unused in most systems).
    uint32_t ss2;     // Stack segment for ring 2 (unused in most systems).
    uint32_t sr3;     // Stack pointer for ring 3 (user mode).
    uint32_t eip;     // Instruction pointer. The next instruction to be executed.
    uint32_t eflags;  // CPU flags. Contains status flags like zero flag, carry flag, etc.
    uint32_t eax;     // General-purpose register `EAX`.
    uint32_t ecx;     // General-purpose register `ECX`.
    uint32_t edx;     // General-purpose register `EDX`.
    uint32_t ebx;     // General-purpose register `EBX`.
    uint32_t esp;     // Stack pointer (`ESP`). Current stack pointer for the task.
    uint32_t ebp;     // Base pointer (`EBP`). Points to the base of the current stack frame.
    uint32_t esi;     // Source Index (`ESI`). Used for string operations.
    uint32_t edi;     // Destination Index (`EDI`). Used for string operations.
    uint32_t es;      // Extra Segment (`ES`). Segment selector for data.
    uint32_t cs;      // Code Segment (`CS`). Segment selector for code.
    uint32_t ss;      // Stack Segment (`SS`). Segment selector for stack.
    uint32_t ds;      // Data Segment (`DS`). Segment selector for data.
    uint32_t fs;      // Additional Segment (`FS`). Can be used for various purposes.
    uint32_t gs;      // Additional Segment (`GS`). Often used for thread-local storage.
    uint32_t ldtr;    // Local Descriptor Table Register (`LDTR`). Holds the selector for the LDT.
    uint32_t iopb;    // I/O port base address. Controls access to I/O ports.
} __attribute__((packed));

void tss_load(int tss_segment);



#endif