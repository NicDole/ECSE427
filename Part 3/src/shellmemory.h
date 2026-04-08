#include <stdio.h>

// ---------------------------------------------------------------------------
// Frame store sizing — set at compile time via:
//   make mysh framesize=X varmemsize=Y
// where X = total lines in the frame store, Y = total variable store slots.
// Defaults allow the code to compile without flags during development.
// ---------------------------------------------------------------------------
#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 300   // default: 100 frames × 3 lines
#endif

#ifndef VAR_STORE_SIZE
#define VAR_STORE_SIZE 1000    // default
#endif

#define FRAME_SIZE  3                          // lines per frame (fixed by assignment)
#define FRAME_COUNT (FRAME_STORE_SIZE / FRAME_SIZE)  // total number of frames

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
