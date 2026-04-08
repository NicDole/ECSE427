#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "shellmemory.h"

#define true  1
#define false 0


// ---------------------------------------------------------------------------
// Frame store
// Frame f occupies physical slots [f*FRAME_SIZE .. f*FRAME_SIZE+FRAME_SIZE-1]
// ---------------------------------------------------------------------------

struct frame_slot {
    int   in_use;
    char *line;
};

// Global, zero-initialized by C runtime (in_use=0, line=NULL for all slots)
static struct frame_slot framestore[FRAME_COUNT * FRAME_SIZE];
static int next_free_frame = 0;


// Free all frame contents and reset the bump allocator.
// Called at the start of each fresh (non-background) exec.
void reset_framestore(void) {
    for (int i = 0; i < FRAME_COUNT * FRAME_SIZE; i++) {
        if (framestore[i].line != NULL) {
            free(framestore[i].line);
            framestore[i].line = NULL;
        }
        framestore[i].in_use = false;
    }
    next_free_frame = 0;
}

// Allocate the next free frame and fill it with up to FRAME_SIZE lines.
// lines[i] == NULL means this slot is padding (script ended before frame was full).
// Returns the frame number, or -1 if no frames remain.
int allocate_frame(const char *lines[FRAME_SIZE]) {
    if (next_free_frame >= FRAME_COUNT) {
        return -1;
    }
    int f    = next_free_frame++;
    int base = f * FRAME_SIZE;
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (lines[i] != NULL) {
            framestore[base + i].line   = strdup(lines[i]);
            framestore[base + i].in_use = true;
        } else {
            framestore[base + i].line   = NULL;
            framestore[base + i].in_use = false;
        }
    }
    return f;
}

// Return the line at the given physical index.
// The physical index is computed by pcb_next_instruction as: frame*FRAME_SIZE + offset.
const char *get_line(size_t physical_index) {
    assert(physical_index < (size_t)(FRAME_COUNT * FRAME_SIZE));
    assert(framestore[physical_index].in_use);
    return framestore[physical_index].line;
}


// ---------------------------------------------------------------------------
// Variable store (unchanged from A2)
// ---------------------------------------------------------------------------

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

void mem_init() {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var   = "none";
        shellmemory[i].value = "none";
    }
}

void mem_set_value(char *var_in, char *value_in) {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
            shellmemory[i].var   = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }
}

char *mem_get_value(char *var_in) {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            return strdup(shellmemory[i].value);
        }
    }
    return NULL;
}
