// #define DEBUG 1


#ifdef DEBUG
#   define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#   define debug(...)
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>              // tolower, isdigit
#include <dirent.h>             // scandir
#include <unistd.h>             // chdir
#include <sys/stat.h>           // mkdir
#include <stdbool.h>            // bool
// for run:
#include <sys/types.h>          // pid_t
#include <sys/wait.h>           // waitpid

#include "shellmemory.h"
#include "shell.h"


#include "pcb.h"
#include "queue.h"
#include "schedule_policy.h"


#include <pthread.h>

// DEFINITIONS
#define true 1
#define false 0
#define MAX_ARGS_SIZE 7
#define MAX_WORKERS 2

// GLOBAL PARAMS

bool threads_created = false;
pthread_t workers[MAX_WORKERS];
static bool quit_when_empty = false; // for threads
static pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER; // queue lock
static pthread_cond_t q_cond = PTHREAD_COND_INITIALIZER;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int badcommandMkdir() {
    printf("Bad command: my_mkdir\n");
    return 4;
}

int badcommandCd() {
    printf("Bad command: my_cd\n");
    return 5;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int echo(char *tok);
int ls();
int my_mkdir(char *name);
int touch(char *path);
int cd(char *path);
int source(char *script);
int my_exec(char *args[], int args_size, bool MT);
int run(char *args[], int args_size);
int badcommandFileDoesNotExist();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    // these bits of debug output were very helpful for debugging
    // the changes we made to the parser!
    debug("#args: %d\n", args_size);
#ifdef DEBUG
    for (size_t i = 0; i < args_size; ++i) {
        debug("  %ld: %s\n", i, command_args[i]);
    }
#endif

    if (args_size < 1) {
        // This shouldn't be possible but we are defensive programmers.
        fprintf(stderr, "interpreter called with no words?\n");
        exit(1);
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1)
            return badcommand();
        return ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2)
            return badcommand();
        return touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return cd(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);
        // a2
    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 2) return badcommand();
        bool MT = false;
		if(strcmp(command_args[args_size-1], "MT")==0){
			MT=true;
			args_size--;
		}
		return my_exec(&command_args[1], args_size - 1, MT);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2)
            return badcommand();
        return run(&command_args[1], args_size - 1);

    } else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int scheduler_is_worker_thread(void) {
    if (!threads_created) {
        return 0;
    }
    pthread_t self = pthread_self();
    return pthread_equal(self, workers[0]) || pthread_equal(self, workers[1]);
}

int quit() {
    if (threads_created){
        quit_when_empty = true;
        // if caller is thread, if so, do not attempt to join on self,
        // return without exiting
        if (scheduler_is_worker_thread()) {
            return 0;
        }
        else{
            // main thread, need to wait for workers to finish
            pthread_cond_broadcast(&q_cond); // Wake everyone up to die
            for (size_t i = 0; i < MAX_WORKERS; ++i) {
                pthread_join(workers[i], NULL);
            }
        }
    }
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    char *value = mem_get_value(var);
    if (value) {
        printf("%s\n", value);
        free(value);
    } else {
        printf("Variable does not exist\n");
    }
    return 0;
}

int echo(char *tok) {
    int must_free = 0;
    // is it a var?
    if (tok[0] == '$') {
        tok++;                  // advance pointer, so that tok is now the stuff after '$'
        tok = mem_get_value(tok);
        if (tok == NULL) {
            tok = "";           // must use empty string, can't pass NULL to printf
        } else {
            must_free = 1;
        }
    }

    printf("%s\n", tok);

    // memory management technically optional for this assignment
    if (must_free) free(tok);

    return 0;
}

// We can hide dotfiles in ls using either the filter operand to scandir,
// or by checking the first character ourselves when we go to print
// the names. That would work, and is less code, but this is more robust.
// And this is also better since it won't allocate extra dirents.
int ls_filter(const struct dirent *d) {
    if (d->d_name[0] == '.') return 0;
    return 1;
}

int ls_compare_char(char a, char b) {
    // assumption: a,b are both either digits or letters.
    // If this is not true, the characters will be effectively compared
    // as ASCII when we do the lower_a - lower_b fallback.

    // if both are digits, compare them
    if (isdigit(a) && isdigit(b)) {
        return a - b;
    }
    // if only a is a digit, then b isn't, so a wins.
    if (isdigit(a)) {
        return -1;
    }

    // lowercase both letters so we can compare their alphabetic position.
    char lower_a = tolower(a), lower_b = tolower(b);
    if (lower_a == lower_b) {
        // a and b are the same letter, possibly in different cases.
        // If they are really the same letter, this returns 0.
        // Otherwise, it's negative if A was capital,
        // and positive if B is capital.
        return a - b;
    }

    // Otherwise, compare their alphabetic position by comparing
    // them at a known case.
    return lower_a - lower_b;
}

int ls_compare_str(const char *a, const char *b) {
    // a simple strcmp implementation that uses ls_compare_char.
    // We only check if *a is zero, since if *b is zero earlier,
    // it would've been unequal to *a at that time and we would return.
    // If *b is zero at the same point or later than *a, we'll exit the
    // loop and return the correct value with the last comparison.

    while (*a != '\0') {
        int d = ls_compare_char(*a, *b);
        if (d != 0) return d;
        a++, b++;
    }
    return ls_compare_char(*a, *b);
}

int ls_compare(const struct dirent **a, const struct dirent **b) {
    return ls_compare_str((*a)->d_name, (*b)->d_name);
}

int ls() {
    // straight out of the man page examples for scandir
    // alphasort uses strcoll instead of strcmp,
    // so we have to implement our own comparator to match the ls spec.
    // Note that the test cases weren't very picky about the specified order,
    // so if you just used alphasort with scandir, you should have passed.
    // This was intentional on our part.
    struct dirent **namelist;
    int n;

    n = scandir(".", &namelist, NULL, ls_compare);
    if (n == -1) {
        // something is catastrophically wrong, just give up.
        perror("my_ls couldn't scan the directory");
        return 0;
    }

    for (size_t i = 0; i < n; ++i) {
        printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);

    return 0;
}

int str_isalphanum(char *name) {
    for (char c = *name; c != '\0'; c = *++name) {
        if (!(isdigit(c) || isalpha(c))) return 0;
    }
    return 1;
}

int my_mkdir(char *name) {
    int must_free = 0;

    debug("my_mkdir: ->%s<-\n", name);

    if (name[0] == '$') {
        ++name;
        // lookup name
        name = mem_get_value(name);
        debug("  lookup: %s\n", name ? name : "(NULL)");
        if (name) {
            // name exists, should free whatever we got
            must_free = 1;
        }
    }
    if (!name || !str_isalphanum(name)) {
        // either name doesn't exist, or isn't valid, error.
        if (must_free) free(name);
        return badcommandMkdir();
    }
    // at this point name is definitely OK

    // 0777 means "777 in octal," aka 511. This value means
    // "give the new folder all permissions that we can."
    int result = mkdir(name, 0777);

    if (result) {
        // description doesn't specify what to do in this case,
        // (including if the directory already exists)
        // so we just give an error message on stderr and ignore it.
        perror("Something went wrong in my_mkdir");
    }

    if (must_free) free(name);
    return 0;
}

int touch(char *path) {
    // we're told we can assume this.
    assert(str_isalphanum(path));
    // if things go wrong, just ignore it.
    FILE *f = fopen(path, "a");
    fclose(f);
    return 0;
}

int cd(char *path) {
    // we're told we can assume this.
    assert(str_isalphanum(path));

    int result = chdir(path);
    if (result) {
        // chdir can fail for several reasons, but the only one we need
        // to handle here for the spec is the ENOENT reason,
        // aka Error NO ENTry -- the directory doesn't exist.
        // Since that's the only one we have to handle, we'll just assume
        // that that's what happened.
        // Alternatively, you can check if the directory exists
        // explicitly first using `stat`. However it is often better to
        // simply try to use a filesystem resource and then recover when
        // you can't, rather than trying to validate first. If you validate
        // first while two users are on the system, there's a race condition!
        return badcommandCd();
    }
    return 0;
}

// source reuses the exec/paging infrastructure: it creates a single PCB
// with FCFS scheduling, so demand paging works for source too.
int source(char *script) {
    char *args[2] = {script, "FCFS"};
    return my_exec(args, 2, false);
}


static struct queue *q = NULL;
static const struct schedule_policy *policy = NULL;

// ---------------------------------------------------------------------------
// Reverse map: frame_owners[frame_number] tells us which program and which
// logical page currently occupies that frame. We need this so that when a
// frame is evicted, we can walk all PCBs and invalidate any pagetable entry
// that pointed to that frame. This is especially important when multiple
// processes run the same script and share frames.
// ---------------------------------------------------------------------------
struct frame_owner {
    char  *filename;  // name of the program that owns this frame
    size_t page_num;  // which logical page of that program is stored here
};
static struct frame_owner frame_owners[FRAME_COUNT];

// ---------------------------------------------------------------------------
// Demand paging: page fault detection and handling (Section 1.2.2)
// ---------------------------------------------------------------------------

// Backing store: an in-memory copy of every line from every loaded program.
// When a program is loaded via exec/source, we read the entire script file
// into a backing_entry. This way, when a page fault occurs later, we can
// fetch the missing page's lines directly from memory without re-opening
// the file.
#define MAX_BACKING_PROGRAMS  10
#define MAX_LINES_PER_PROGRAM 200

struct backing_entry {
    char  *filename;                       // name of the script file
    char  *lines[MAX_LINES_PER_PROGRAM];   // all lines of the script, strdup'd
    size_t line_count;                     // total number of lines in the script
    size_t page_count;                     // ceil(line_count / FRAME_SIZE)
};

// Forward declaration — full implementation defined later alongside backing_store.
static struct backing_entry *find_backing_entry(const char *filename);

// Before executing each instruction, we check whether the page it lives on
// is loaded. If pagetable[page] == -1, the page is not in memory and we
// need to trigger a page fault.
static int next_page_is_loaded(struct PCB *pcb) {
    size_t page = pcb->pc / FRAME_SIZE;
    return pcb->pagetable[page] != -1;
}

// Page fault handler (Section 1.2.2).
//
// Called when a process tries to execute an instruction whose page is not
// in the frame store (pagetable entry == -1). This function:
//   1. Looks up the missing page's lines in the backing store (no file I/O).
//   2. If a free frame exists, loads the page there.
//   3. If the frame store is full, picks the LRU victim frame, prints its
//      contents, evicts it, loads the new page, and invalidates any PCBs
//      that were referencing the evicted frame.
// The faulting process is then sent to the back of the ready queue by the
// caller (run_pcb_for_n_steps / run_pcb_to_completion).
static void handle_page_fault(struct PCB *pcb) {
    size_t page = pcb->pc / FRAME_SIZE;

    // Fetch the missing page's lines from the backing store — this is why
    // we keep an in-memory copy of the whole script, so we never need to
    // re-open the file.
    struct backing_entry *entry = find_backing_entry(pcb->name);
    const char *lines[FRAME_SIZE];
    for (int i = 0; i < FRAME_SIZE; i++) {
        size_t idx = page * FRAME_SIZE + i;
        lines[i] = (idx < entry->line_count) ? entry->lines[idx] : NULL;
    }

    // Try to allocate a free frame for this page.
    int frame = allocate_frame(lines);

    if (frame >= 0) {
        // Free frame was available — just load the page and update the
        // page table and reverse map.
        printf("Page fault!\n");
        pcb->pagetable[page] = frame;
        free(frame_owners[frame].filename);
        frame_owners[frame].filename = strdup(pcb->name);
        frame_owners[frame].page_num = page;
    } else {
        // Frame store is full — must evict the least recently used frame.
        int victim = find_lru_frame();

        // Print victim page contents before overwriting the frame.
        printf("Page fault! Victim page contents:\n");
        printf("\n");
        for (int i = 0; i < FRAME_SIZE; i++) {
            const char *line = get_frame_line(victim, i);
            if (line) printf("%s", line);
        }
        printf("\n");
        printf("End of victim page contents.\n");

        // Invalidation: use the reverse map (frame_owners) to find which
        // program/page occupied this frame, then walk all PCBs in the
        // ready queue and set their pagetable entry to -1. This is critical
        // when two processes run the same script — they share frames, so
        // both must be invalidated when a shared frame is evicted.
        char  *victim_fname = frame_owners[victim].filename;
        size_t victim_page  = frame_owners[victim].page_num;

        if (victim_fname != NULL) {
            struct PCB *p = queue_peek_head(q);
            while (p) {
                if (strcmp(p->name, victim_fname) == 0)
                    p->pagetable[victim_page] = -1;
                p = p->next;
            }
            // Also invalidate the faulting PCB itself if it shared this frame.
            if (strcmp(pcb->name, victim_fname) == 0)
                pcb->pagetable[victim_page] = -1;
        }

        // Load the new page into the evicted frame slot and update bookkeeping.
        replace_frame(victim, lines);
        pcb->pagetable[page] = victim;

        free(frame_owners[victim].filename);
        frame_owners[victim].filename = strdup(pcb->name);
        frame_owners[victim].page_num = page;
    }
}

void runSchedule(struct queue *q, const struct schedule_policy *policy) {
    struct PCB *next_pcb = policy->dequeue(q);
    while (next_pcb) {
        next_pcb = policy->run_pcb(next_pcb);
        if (next_pcb) policy->enqueue(q, next_pcb);
        next_pcb = policy->dequeue(q);
    }
}

// FCFS scheduling: run until done. Same page fault logic as RR — if the
// next page isn't loaded, handle the fault and re-enqueue.
struct PCB *run_pcb_to_completion(struct PCB *pcb) {
    while (pcb_has_next_instruction(pcb)) {
        if (!next_page_is_loaded(pcb)) {
            handle_page_fault(pcb);
            return pcb;  // page fault: interrupt and re-enqueue
        }
        size_t instr = pcb_next_instruction(pcb);
        parseInput(get_line(instr));
    }
    free_pcb(pcb);
    return NULL;
}

// RR scheduling: run up to n instructions. On a page fault, the process is
// interrupted and returned to the caller, which places it at the back of the
// ready queue. The missing page is loaded by handle_page_fault, so when
// the process comes back around in the queue, its page will be ready.
struct PCB *run_pcb_for_n_steps(struct PCB *pcb, size_t n) {
    debug("run n steps: n is %ld\n", n);
    for (; n && pcb_has_next_instruction(pcb); --n) {
        if (!next_page_is_loaded(pcb)) {
            handle_page_fault(pcb);
            return pcb;  // page fault: interrupt and re-enqueue
        }
        parseInput(get_line(pcb_next_instruction(pcb)));
    }
    debug("run n steps: looped to %ld\n", n);
    if (pcb_has_next_instruction(pcb)) {
        return pcb;
    } else {
        free_pcb(pcb);
        return NULL;
    }
}



void *scheduler_worker(void *arg) {
    // what the thread will do until it dies
    while (1) {
        pthread_mutex_lock(&q_mutex);

        while (!quit_when_empty && is_queue_empty(q)) {
            // smart sleeping
            pthread_cond_wait(&q_cond, &q_mutex);
        }

        if (quit_when_empty && is_queue_empty(q)) {
            pthread_mutex_unlock(&q_mutex);
            pthread_exit(NULL);
        }

        struct PCB *pcb = policy->dequeue(q);
        pthread_mutex_unlock(&q_mutex);

        if (!pcb) continue;

        pcb = policy->run_pcb(pcb);

        if (pcb) {
            pthread_mutex_lock(&q_mutex);
            policy->enqueue(q, pcb);
            pthread_cond_signal(&q_cond);
            pthread_mutex_unlock(&q_mutex);
        }
    }
    return NULL;
}

void create_threads(void) {
    if (threads_created) return;

    threads_created = true;

    for (size_t i = 0; i < MAX_WORKERS; ++i) {
        pthread_create(&workers[i], NULL, scheduler_worker, NULL);
    }
}



// ---------------------------------------------------------------------------
// Backing store: the global array that holds every loaded program's lines.
// Persists across PCB lifetimes so page faults can fetch lines from memory
// without re-opening any files. Reset at the start of each fresh exec.
// ---------------------------------------------------------------------------
static struct backing_entry backing_store[MAX_BACKING_PROGRAMS];
static int backing_count = 0;

static void reset_backing_store(void) {
    for (int i = 0; i < backing_count; i++) {
        free(backing_store[i].filename);
        backing_store[i].filename = NULL;
        for (size_t j = 0; j < backing_store[i].line_count; j++) {
            free(backing_store[i].lines[j]);
            backing_store[i].lines[j] = NULL;
        }
        backing_store[i].line_count = 0;
        backing_store[i].page_count = 0;
    }
    backing_count = 0;
}

// Look up an entry in the backing store by filename. Returns NULL if not found.
static struct backing_entry *find_backing_entry(const char *filename) {
    for (int i = 0; i < backing_count; i++) {
        if (strcmp(backing_store[i].filename, filename) == 0)
            return &backing_store[i];
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Process sharing (Section 1.2.1): when exec receives the same script name
// twice (e.g. "exec prog1 prog1 RR"), the file is only loaded once. Both
// PCBs get their own copy of the pagetable, but they point to the same
// physical frames. This cache tracks which files have already been loaded
// during the current exec call.
// ---------------------------------------------------------------------------
#define MAX_UNIQUE_PROGRAMS 4

struct loaded_program {
    const char *filename;
    int        *pagetable;
    size_t      page_count;
    size_t      line_count;
};

// Lazy loading (Section 1.2.2):
// 1. Reads ALL lines of a script into the backing store (in-memory file copy).
// 2. Only loads the first 2 pages (first 6 lines) into actual frame store slots.
// 3. All remaining pagetable entries are set to -1, meaning "not yet loaded."
//    Those pages will be brought in on demand by the page fault handler.
// Returns a malloc'd pagetable array on success, or NULL on failure.
static int *load_file_into_frames(const char *filename,
                                   size_t *out_page_count,
                                   size_t *out_line_count) {
    FILE *f = fopen(filename, "rt");
    if (!f) return NULL;

    if (backing_count >= MAX_BACKING_PROGRAMS) {
        fprintf(stderr, "Error: too many programs loaded\n");
        fclose(f);
        return NULL;
    }

    // Read every single line of the script into the backing store.
    // This is our in-memory copy of the file — the page fault handler
    // will pull lines from here instead of re-opening the file.
    struct backing_entry *entry = &backing_store[backing_count++];
    entry->filename   = strdup(filename);
    entry->line_count = 0;
    entry->page_count = 0;

    char buf[MAX_USER_INPUT];
    while (entry->line_count < MAX_LINES_PER_PROGRAM && !feof(f)) {
        memset(buf, 0, MAX_USER_INPUT);
        if (fgets(buf, MAX_USER_INPUT, f) == NULL) break;
        entry->lines[entry->line_count++] = strdup(buf);
    }
    fclose(f);

    entry->page_count = (entry->line_count + FRAME_SIZE - 1) / FRAME_SIZE;

    // Initialize the pagetable: every entry starts as -1 ("not loaded").
    // Only the first 2 pages will be eagerly loaded below.
    int *pagetable = malloc(entry->page_count * sizeof(int));
    if (!pagetable) return NULL;
    for (size_t p = 0; p < entry->page_count; p++) pagetable[p] = -1;

    // Eagerly load only the first 2 pages (or 1 if the script is < 3 lines).
    // Everything beyond page 2 stays as -1 and will trigger a page fault
    // when the process eventually tries to execute those instructions.
    size_t pages_to_load = entry->page_count < 2 ? entry->page_count : 2;
    for (size_t p = 0; p < pages_to_load; p++) {
        const char *lines[FRAME_SIZE];
        for (int i = 0; i < FRAME_SIZE; i++) {
            size_t idx = p * FRAME_SIZE + i;
            lines[i] = (idx < entry->line_count) ? entry->lines[idx] : NULL;
        }
        int frame = allocate_frame(lines);
        if (frame < 0) {
            fprintf(stderr, "Error: frame store is full during initial load\n");
            free(pagetable);
            return NULL;
        }
        pagetable[p] = frame;

        // Update the reverse map so eviction knows who owns this frame.
        free(frame_owners[frame].filename);
        frame_owners[frame].filename = strdup(filename);
        frame_owners[frame].page_num = p;
    }

    *out_page_count = entry->page_count;
    *out_line_count = entry->line_count;
    return pagetable;
}

int my_exec(char *args[], int args_size, bool MT) {
    assert(args_size >= 2);
    (void)MT; // multithreading not used in A3

    if (args_size < 2 || args_size > 4) {
        return badcommand();
    }
    char *policy_name = args[args_size-1];
    args_size--;
    policy = get_policy(policy_name);
    if (!policy) {
        printf("Bad command: unknown scheduling policy\n");
        return 1;
    }

    // Fresh exec: reset the frame store, backing store, reverse map, and queue.
    reset_framestore();
    reset_backing_store();
    for (int i = 0; i < FRAME_COUNT; i++) {
        free(frame_owners[i].filename);
        frame_owners[i].filename = NULL;
        frame_owners[i].page_num = 0;
    }
    assert(!q);
    q = alloc_queue();

    // Per-exec cache so the same filename is only loaded once into frames.
    struct loaded_program cache[MAX_UNIQUE_PROGRAMS];
    int cache_size = 0;

    for (int n = 0; n < args_size; ++n) {
        const char *fname = args[n];

        // Check if this filename was already loaded during this exec call.
        struct loaded_program *prog = NULL;
        for (int c = 0; c < cache_size; c++) {
            if (strcmp(cache[c].filename, fname) == 0) {
                prog = &cache[c];
                break;
            }
        }

        if (prog == NULL) {
            // Not yet loaded: read the file and fill frames.
            size_t page_count, line_count;
            int *pagetable = load_file_into_frames(fname, &page_count, &line_count);
            if (!pagetable) {
                printf("Bad command: File not found\n");
                goto cleanup;
            }
            cache[cache_size].filename   = fname;
            cache[cache_size].pagetable  = pagetable;
            cache[cache_size].page_count = page_count;
            cache[cache_size].line_count = line_count;
            prog = &cache[cache_size];
            cache_size++;
        }

        // Each PCB gets its own copy of the pagetable int[] so that
        // free_pcb can safely free it even when two PCBs share the same frames.
        int *pt_copy = malloc(prog->page_count * sizeof(int));
        memcpy(pt_copy, prog->pagetable, prog->page_count * sizeof(int));

        struct PCB *pcb = create_process_paged(fname, pt_copy,
                                                prog->page_count, prog->line_count);
        if (!pcb) {
            free(pt_copy);
            printf("Failed to create process\n");
            goto cleanup;
        }

        policy->enqueue(q, pcb);
    }

    // Free the original (cache) pagetables — PCBs have their own copies now.
    for (int c = 0; c < cache_size; c++) {
        free(cache[c].pagetable);
        cache[c].pagetable = NULL;
    }

    runSchedule(q, policy);

done:
    free_queue(q);
    q = NULL;
    policy = NULL;
    return 0;

cleanup:
    // Free any cache pagetables that haven't been handed off to PCBs yet.
    for (int c = 0; c < cache_size; c++) {
        if (cache[c].pagetable) free(cache[c].pagetable);
    }
    goto done;
}


int run(char *args[], int arg_size) {
    // copy the args into a new NULL-terminated array.
    char **adj_args = calloc(arg_size + 1, sizeof(char *));
    for (int i = 0; i < arg_size; ++i) {
        adj_args[i] = args[i];
    }

    // always flush output streams before forking.
    fflush(stdout);
    // attempt to fork the shell
    pid_t pid = fork();
    if (pid < 0) {
        // fork failed. Report the error and move on.
        perror("fork() failed");
        return 1;
    } else if (pid == 0) {
        // we are the new child process.
        execvp(adj_args[0], adj_args);
        perror("exec failed");
        // The parent and child are sharing stdin, and according to
        // a part of the glibc documentation that you are **not**
        // expected to know for this course, a shared input handle
        // should be fflushed (if it is needed) or closed
        // (if it is not). Handling this exec error case is not even
        // necessary, but let's do it right.
        // (Failure to do this can result in the parent process
        // reading the remaining input twice in batch mode.)
        fclose(stdin);
        exit(1);
    } else {
        // we are the parent process.
        waitpid(pid, NULL, 0);
    }
    free(adj_args);
    return 0;
}
