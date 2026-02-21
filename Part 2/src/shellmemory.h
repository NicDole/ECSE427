#define MEM_SIZE 1000

/**
 * Initialize shell memory structures
 */
void mem_init();

/**
 * Get value associated with a variable name.
 *
 * @param var Variable name to lookup
 * @return Duplicated string value, or NULL if not found
 */
char *mem_get_value(char *var);

/**
 * Set a variable-value pair in shell memory.
 *
 * @param var   Variable name
 * @param value Value to assign
 */
void mem_set_value(char *var, char *value);

/**
 * Load entire program from file into shell memory.
 *
 * Reads all lines from the file and stores them for later execution.
 * Automatically clears any existing program before loading.
 *
 * @param filename Path to the script file to load
 * @return Number of lines loaded on success, -1 on error
 */
int mem_load_program(char *filename);

/**
 * Get a program line by index.
 *
 * @param index Zero-based index of the line to retrieve
 * @return Pointer to the line string, or NULL if index is invalid
 */
char *mem_get_program_line(int index);

/**
 * Get the total number of program lines currently loaded.
 *
 * @return Number of lines in program memory
 */
int mem_get_program_line_count(void);

/**
 * Clear all program lines from memory and free allocated strings.
 *
 * Resets the program line counter to zero. Should be called when
 * a program finishes execution to free memory.
 */
void mem_clear_program(void);

/**
 * Append program lines from file to current program memory (no clear).
 * Used by exec to load multiple scripts contiguously.
 *
 * @param filename Path to the script file to load
 * @return Number of lines appended on success, -1 on error (file not found or out of space)
 */
int mem_append_program(char *filename);
