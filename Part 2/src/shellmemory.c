#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

/** Program line storage - array of strings holding loaded program lines */
char *program_lines[MEM_SIZE];

/** Counter tracking number of program lines currently loaded */
int program_line_count = 0;

/**
 * @brief Helper function to match variable names
 * @param model First string to compare
 * @param var Second string to compare
 * @return 1 if strings match, 0 otherwise
 */
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i])
            matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else
        return 0;
}

void mem_init() {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
    // Initialize program line storage
    program_line_count = 0;
    for (i = 0; i < MEM_SIZE; i++) {
        program_lines[i] = NULL;
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

    // Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    return;
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

int mem_load_program(char *filename) {
    FILE *p = fopen(filename, "rt");
    if (p == NULL) {
        return -1;
    }

    char line[101];  // Max line length is 100 chars + null terminator
    int lines_loaded = 0;

    // Clear any existing program first
    mem_clear_program();

    // Read all lines from file
    while (fgets(line, sizeof(line), p) != NULL && program_line_count < MEM_SIZE) {
        // Remove newline if present
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
        }

        // Store the line (duplicate the string)
        program_lines[program_line_count] = strdup(line);
        program_line_count++;
        lines_loaded++;
    }

    fclose(p);
    return lines_loaded;
}

char *mem_get_program_line(int index) {
    if (index < 0 || index >= program_line_count) {
        return NULL;
    }
    return program_lines[index];
}

int mem_get_program_line_count(void) {
    return program_line_count;
}

void mem_clear_program(void) {
    int i;
    for (i = 0; i < program_line_count; i++) {
        if (program_lines[i] != NULL) {
            free(program_lines[i]);
            program_lines[i] = NULL;
        }
    }
    program_line_count = 0;
}
