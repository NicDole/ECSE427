#include <stdlib.h>
#include "scheduler.h"
#include "shellmemory.h"

// Global ready queue for FCFS scheduling
static struct ReadyQueue ready_queue;
// Auto-incrementing PID counter
static int next_pid = 1;

void ready_queue_init(void) {
    ready_queue.head = NULL;
    ready_queue.tail = NULL;
}

void ready_queue_enqueue(struct PCB *pcb) {
    if (pcb == NULL) {
        return;
    }

    // Clear next pointer since this will be last element
    pcb->next = NULL;

    if (ready_queue.head == NULL) {
        // Queue is empty - this PCB becomes both head and tail
        ready_queue.head = pcb;
        ready_queue.tail = pcb;
    } else {
        // Add to tail for FIFO order
        ready_queue.tail->next = pcb;
        ready_queue.tail = pcb;
    }
}

struct PCB *ready_queue_dequeue(void) {
    if (ready_queue.head == NULL) {
        return NULL;
    }

    // Get PCB at head (oldest process)
    struct PCB *pcb = ready_queue.head;
    // Move head to next PCB
    ready_queue.head = ready_queue.head->next;

    if (ready_queue.head == NULL) {
        // Queue became empty - clear tail too
        ready_queue.tail = NULL;
    }

    // Clear next pointer before returning
    pcb->next = NULL;
    return pcb;
}

int ready_queue_is_empty(void) {
    return ready_queue.head == NULL;
}

struct PCB *pcb_create(int start_index, int length) {
    struct PCB *pcb = (struct PCB *)malloc(sizeof(struct PCB));
    if (pcb == NULL) {
        return NULL;
    }

    // Assign unique PID and increment counter
    pcb->pid = next_pid++;
    // Store program location in shell memory
    pcb->start_index = start_index;
    pcb->length = length;
    // Start at first instruction
    pcb->pc = 0;
    pcb->job_length_score = length;
    pcb->next = NULL;

    return pcb;
}

void pcb_free(struct PCB *pcb) {
    if (pcb != NULL) {
        free(pcb);
    }
}

int pcb_is_done(struct PCB *pcb) {
    if (pcb == NULL) {
        return 1;
    }
    // Process done when PC reaches program length
    return pcb->pc >= pcb->length;
}

char *pcb_get_current_instruction(struct PCB *pcb) {
    if (pcb == NULL || pcb_is_done(pcb)) {
        return NULL;
    }

    // Calculate actual index: start_index + program counter
    int actual_index = pcb->start_index + pcb->pc;
    return mem_get_program_line(actual_index);
}

void pcb_advance(struct PCB *pcb) {
    if (pcb != NULL && !pcb_is_done(pcb)) {
        // Move to next instruction
        pcb->pc++;
    }
}

int scheduler_quantum(SchedulePolicy policy) {
    switch (policy) {
    case POLICY_FCFS:
    case POLICY_SJF:
        return 0;   // run to completion
    case POLICY_RR:
        return 2;   // time slice 2 (1.2.3)
    case POLICY_RR30:
        return 30;  // time slice 30
    case POLICY_AGING:
        return 1;   // time slice 1 (1.2.4)
    }
    return 0;
}

void ready_queue_age(void) {
    struct PCB *p = ready_queue.head;
    while (p != NULL) {
        if (p->job_length_score > 0) {
            p->job_length_score--;
        }
        p = p->next;
    }
}

void ready_queue_enqueue_aging(struct PCB *pcb, int reinsert) {
    if (pcb == NULL) {
        return;
    }
    pcb->next = NULL;

    if (ready_queue.head == NULL) {
        ready_queue.head = pcb;
        ready_queue.tail = pcb;
        return;
    }

    // Re-insert if no job in queue has a lower score, job that just ran runs again.
    if (reinsert && ready_queue.head->job_length_score >= pcb->job_length_score) {
        pcb->next = ready_queue.head;
        ready_queue.head = pcb;
        return;
    }

    // Initially we will insert before first with score > pcb's.
    // When we reinsert we will insert before first with score >= pcb's (someone had lower score).
    if (!reinsert && ready_queue.head->job_length_score > pcb->job_length_score) {
        pcb->next = ready_queue.head;
        ready_queue.head = pcb;
        return;
    }

    struct PCB *prev = ready_queue.head;
    struct PCB *cur = ready_queue.head->next;
    while (cur != NULL) {
        if (reinsert) {
            if (cur->job_length_score >= pcb->job_length_score) {
                break;
            }
        } else {
            if (cur->job_length_score > pcb->job_length_score) {
                break;
            }
        }
        prev = cur;
        cur = cur->next;
    }
    prev->next = pcb;
    pcb->next = cur;
    if (cur == NULL) {
        ready_queue.tail = pcb;
    }
}


void ready_queue_enqueue_front(struct PCB *pcb) {
    if (pcb == NULL) return;

    pcb->next = ready_queue.head;
    ready_queue.head = pcb;

    if (ready_queue.tail == NULL) {
        ready_queue.tail = pcb;
    }
}