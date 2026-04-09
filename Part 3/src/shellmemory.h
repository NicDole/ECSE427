#include <stdio.h>

// ---------------------------------------------------------------------------
// Compile-time memory configuration (Section 1.2.2)
//
// The frame store size and variable store size are set at compile time:
//   make mysh framesize=X varmemsize=Y
//
// The Makefile passes these as -D flags to gcc, which replace the macros
// FRAME_STORE_SIZE and VAR_STORE_SIZE with the numeric values X and Y.
// At startup, the shell prints these sizes instead of the old version message.
// ---------------------------------------------------------------------------
#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 300
#endif

#ifndef VAR_STORE_SIZE
#define VAR_STORE_SIZE 1000
#endif

// Each page/frame holds exactly 3 lines of code.
// The total number of frames is the frame store size divided by 3.
#define FRAME_SIZE  3
#define FRAME_COUNT (FRAME_STORE_SIZE / FRAME_SIZE)

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
// Call at the start of each fresh exec.
void reset_framestore(void);

// Returns the line at (frame_num, offset) without any assertion.
// Returns NULL if that slot is unused (padding or evicted).
// Used to print victim page contents before eviction.
const char *get_frame_line(int frame_num, int offset);

// Overwrites an existing frame slot with new content (frees old strings first).
// Used to load a new page into a victim frame during eviction.
void replace_frame(int frame_num, const char *lines[FRAME_SIZE]);

// Returns the frame index with the smallest LRU timestamp (least recently used).
int find_lru_frame(void);
