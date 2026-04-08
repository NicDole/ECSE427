#pragma once
#include <stddef.h>
#include <stdio.h>

typedef size_t pid;

struct PCB {
    pid     pid;
    char   *name;
    int    *pagetable;   // pagetable[page] = frame number in the frame store; malloc'd
    size_t  page_count;  // number of pages (entries in pagetable)
    size_t  line_count;  // total real lines in the script (for pc termination check)
    size_t  duration;    // used by scheduling policies (initially = line_count)
    size_t  pc;          // logical program counter: 0 .. line_count-1
    struct PCB *next;
};

// pcb_has_next_instruction: returns non-zero if there are more instructions to run
int pcb_has_next_instruction(struct PCB *pcb);

// pcb_next_instruction: translates the current logical pc to a physical frame store
// index, increments pc, and returns the physical index for use with get_line().
size_t pcb_next_instruction(struct PCB *pcb);

// create_process_paged: allocates a new PCB using a pre-loaded pagetable.
// The caller is responsible for passing a malloc'd pagetable copy; free_pcb will free it.
struct PCB *create_process_paged(const char *name, int *pagetable,
                                  size_t page_count, size_t line_count);

// free_pcb: frees the PCB and its pagetable.
// Does NOT free frame store contents — frames persist until reset_framestore().
void free_pcb(struct PCB *pcb);
