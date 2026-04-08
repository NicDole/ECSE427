#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "pcb.h"

// Returns non-zero if the process has more instructions to execute.
int pcb_has_next_instruction(struct PCB *pcb) {
    return pcb->pc < pcb->line_count;
}

// Translates the logical pc to a physical frame store index and advances pc.
// Address translation:
//   page   = pc / FRAME_SIZE        (which virtual page)
//   offset = pc % FRAME_SIZE        (which line within that page)
//   frame  = pagetable[page]         (which physical frame)
//   physical index = frame * FRAME_SIZE + offset
size_t pcb_next_instruction(struct PCB *pcb) {
    size_t page   = pcb->pc / FRAME_SIZE;
    size_t offset = pcb->pc % FRAME_SIZE;
    int    frame  = pcb->pagetable[page];
    pcb->pc++;
    return (size_t)(frame * FRAME_SIZE + offset);
}

// Creates a new PCB from a pre-loaded pagetable.
// pagetable must be a malloc'd int[] owned exclusively by this PCB.
// (Callers that need to share frames across PCBs should pass a memcpy'd copy.)
struct PCB *create_process_paged(const char *name, int *pagetable,
                                  size_t page_count, size_t line_count) {
    struct PCB *pcb = malloc(sizeof(struct PCB));
    static pid fresh_pid = 1;
    pcb->pid        = fresh_pid++;
    pcb->name       = strdup(name);
    pcb->pagetable  = pagetable;
    pcb->page_count = page_count;
    pcb->line_count = line_count;
    pcb->duration   = line_count;
    pcb->pc         = 0;
    pcb->next       = NULL;
    return pcb;
}

// Frees a PCB and its pagetable array.
// Does NOT free frame store contents — those persist across PCB lifetimes
// and are cleaned up by reset_framestore() at the start of the next exec call.
void free_pcb(struct PCB *pcb) {
    free(pcb->pagetable);
    free(pcb->name);
    free(pcb);
}
