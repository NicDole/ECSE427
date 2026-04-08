#include <stdio.h>

// Variable store size
#define MEM_SIZE 1000

// Frame store constants
#define FRAME_SIZE  3    // lines per frame (fixed by assignment)
#define FRAME_COUNT 100  // total frames (static for 1.2.1; becomes compile-time in 1.2.2)

// Variable store API (unchanged from A2)
void  mem_init();
char *mem_get_value(char *var);
void  mem_set_value(char *var, char *value);

// Frame store API
// Allocates the next free frame and fills it with up to FRAME_SIZE lines.
// lines[i] may be NULL for padding (partial last page of a script).
// Returns the frame number (>= 0), or -1 if the frame store is full.
int allocate_frame(const char *lines[FRAME_SIZE]);

// Returns the line at physical address (frame * FRAME_SIZE + offset).
const char *get_line(size_t physical_index);

// Frees all frame store contents and resets the allocator.
// Call at the start of each fresh (non-background) exec.
void reset_framestore(void);
