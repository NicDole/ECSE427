#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * Scheduling policy. Used by exec to select enqueue order and time slice.
 * 0 = run to completion (FCFS, SJF); >0 = max instructions before preempt (RR, AGING).
 */
typedef enum {
    POLICY_FCFS,
    POLICY_SJF,
    POLICY_RR,
    POLICY_AGING
} SchedulePolicy;

/**
 * Process Control Block structure.
 *
 * Tracks execution state for a script/process. Each script loaded into
 * shell memory gets its own PCB to track where it is in execution.
 */
struct PCB {
    int pid;                    // Unique process identifier (auto-assigned, starts at 1)
    int start_index;            // Starting index in shell memory where program lines begin
    int length;                 // Total number of lines in the program
    int pc;                     // Program counter: current instruction index (0-based)
    struct PCB *next;           // Pointer to next PCB in ready queue (for linked list)
};

/**
 * Ready Queue structure.
 *
 * Implements a FIFO queue using a linked list of PCBs. Tracks both head
 * (oldest process) and tail (newest process) for efficient enqueue/dequeue.
 * Used for FCFS (First Come First Served) scheduling.
 */
struct ReadyQueue {
    struct PCB *head;  // Pointer to first PCB in queue (next to execute)
    struct PCB *tail;  // Pointer to last PCB in queue (most recently added)
};

/**
 * Initialize the ready queue.
 */
void ready_queue_init(void);

/**
 * Add a PCB to the tail of the ready queue.
 *
 * @param pcb Pointer to PCB to enqueue
 */
void ready_queue_enqueue(struct PCB *pcb);

/**
 * Remove and return the PCB at the head of the ready queue.
 *
 * @return Pointer to PCB at head, or NULL if queue is empty
 */
struct PCB *ready_queue_dequeue(void);

/**
 * Check if the ready queue is empty.
 *
 * @return 1 if empty, 0 otherwise
 */
int ready_queue_is_empty(void);

/**
 * Create a new PCB for a script.
 *
 * @param start_index Starting index in shell memory
 * @param length      Number of lines in the program
 * @return Pointer to newly allocated PCB, or NULL on failure
 */
struct PCB *pcb_create(int start_index, int length);

/**
 * Free a PCB structure.
 *
 * @param pcb Pointer to PCB to free
 */
void pcb_free(struct PCB *pcb);

/**
 * Check if PCB has completed execution.
 *
 * @param pcb Pointer to PCB to check
 * @return 1 if process is done (pc >= length), 0 otherwise
 */
int pcb_is_done(struct PCB *pcb);

/**
 * Get the current instruction line for a PCB.
 *
 * @param pcb Pointer to PCB
 * @return Pointer to current instruction string, or NULL if invalid
 */
char *pcb_get_current_instruction(struct PCB *pcb);

/**
 * Advance the program counter to the next instruction.
 *
 * @param pcb Pointer to PCB
 */
void pcb_advance(struct PCB *pcb);

/**
 * Number of instructions to run before preempting (for preemptive policies).
 * Returns 0 for non-preemptive (run to completion).
 */
int scheduler_quantum(SchedulePolicy policy);

#endif // SCHEDULER_H
