#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <pthread.h>

/**
 * Scheduling policy. Used by exec to select enqueue order and time slice.
 * 0 = run to completion (FCFS, SJF); >0 = max instructions before preempt (RR, AGING).
 */
typedef enum {
    POLICY_FCFS,
    POLICY_SJF,
    POLICY_RR,
    POLICY_RR30,
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
    int job_length_score;       // For AGING: sort key, aged each time slice (min 0)
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

/**
 * Age all jobs in the ready queue: decrease job_length_score by 1, floor at 0.
 * Used by AGING policy after each time slice (do not age the job that just ran).
 */
void ready_queue_age(void);

/**
 * Enqueue PCB in order of job_length_score.
 * When reinsert is 1 (job that just ran): insert at front if no lower score in queue, else before first with score >= pcb's.
 * When reinsert is 0 (initial load): insert before first with score > pcb's so equal scores preserve order.
 *
 * @param pcb     Pointer to PCB to enqueue
 * @param reinsert 1 when re-inserting after a time slice, 0 when building initial queue
 */
void ready_queue_enqueue_aging(struct PCB *pcb, int reinsert);

/**
 * Enqueue a PCB at the head of the ready queue.
 *
 * Used when a process must run before others already in the queue
 * (example: background batch script process for exec ... #).
 *
 * @param pcb Pointer to PCB to enqueue at front
 */
void ready_queue_enqueue_front(struct PCB *pcb);


/**
 * Initialize thread-safe ready queue support for MT mode.
 *
 * Must be called once before using the *_mt functions.
 */
void ready_queue_mt_init(void);

/**
 * Enqueue a PCB in MT mode (thread-safe). Signals workers that work is available.
 *
 * @param pcb Pointer to PCB to enqueue
 */
void ready_queue_mt_enqueue(struct PCB *pcb);

/**
 * Enqueue a PCB at the head in MT mode (thread-safe).
 *
 * @param pcb Pointer to PCB to enqueue at front
 */
void ready_queue_mt_enqueue_front(struct PCB *pcb);

/**
 * Dequeue a PCB in MT mode (thread-safe, blocking).
 *
 * Blocks until a PCB is available or shutdown is requested.
 *
 * @return Pointer to PCB, or NULL if shutdown and queue is empty
 */
struct PCB *ready_queue_mt_dequeue_blocking(void);

/**
 * Notify the queue that a worker finished a time slice.
 *
 * Used to track when all work is done so the main thread can return.
 */
void ready_queue_mt_worker_done(void);

/**
 * Block until the queue is empty AND no worker is currently running a PCB.
 *
 * Used by exec in MT mode to wait for completion.
 */
void ready_queue_mt_wait_all_done(void);

/**
 * Signal MT workers to shut down (used by quit). Wakes any blocked workers.
 */
void ready_queue_mt_shutdown(void);

#endif // SCHEDULER_H

