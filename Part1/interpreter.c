#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // chdir
#include "shellmemory.h"
#include "shell.h"
#include <ctype.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

// Max number of args for most commands (run can have more)
int MAX_ARGS_SIZE = 3;

// Generic error handler for unknown commands
int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command when file doesnt exist
// Returns 3 to distinguish from other error types (other errors return 1)
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

// Error handler for my_cd when directory change fails
int badcommandMyCd() {
    printf("Bad command: my_cd\n");
    return 1;
}

// Error handler for my_touch when file creation fails
int badcommandMyTouch() {
    printf("Bad command: my_touch\n");
    return 1;
}

// Error handler for my_mkdir when directory creation fails
int badcommandMyMkdir() {
    printf("Bad command: my_mkdir\n");
    return 1;
}

// Checks if string only contains alphanumeric characters
// Returns 1 if valid, 0 if NULL, empty, or contains non-alphanumeric chars
int is_alnum_string(char *s) {
    if (s == NULL || strlen(s) == 0) return 0;
    for (int i = 0; s[i]; i++) {
        if (!isalnum(s[i])) return 0;
    }
    return 1;
}


int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);

int echo_cmd(char *token);
int my_touch(char *filename);
int my_cd(char *dirname);
int my_ls();
int my_mkdir(char *dirname);
int run_cmd(char *command_args[], int args_size);

// Comparison function for qsort to alphabetically sort directory entries
// qsort passes pointers to array elements, so we need to dereference twice
static int cmp_strings(const void *a, const void *b) {
    const char *sa = *(const char * const *)a;  // Cast void* to char**, then dereference
    const char *sb = *(const char * const *)b;
    return strcmp(sa, sb);
}

// Takes parsed command arguments and routes to the right handler
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1) {
        return badcommand();
    }

    // Remove newlines and carriage returns from arguments (strcspn finds first \r or \n, replace with null terminator)
    for (i = 0; i < args_size; i++) {
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }
    
    // Run command can have variable arguments, others cant
    if (args_size > MAX_ARGS_SIZE) {
        if (strcmp(command_args[0], "run") != 0) {
            return badcommand();
        }
    }

    // Route to appropriate command handler based on first argument
    // Each command validates its expected argument count before executing
    if (strcmp(command_args[0], "help") == 0) {
        if (args_size != 1) return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        if (args_size != 1) return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        if (args_size != 3) return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2) return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2) return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2) return badcommand();
        return echo_cmd(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2) return badcommandMyTouch();
        return my_touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2) return badcommandMyCd();
        return my_cd(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1) return badcommand();
        return my_ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2) return badcommandMyMkdir();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2) return badcommand();
        return run_cmd(command_args, args_size);

    } else {
        return badcommand();
    }
}

// Displays all available commands and their descriptions
int help() {
    char help_string[] =
        "COMMAND\t\t\tDESCRIPTION\n \
help\t\t\tDisplays all the commands\n \
quit\t\t\tExits / terminates the shell with \"Bye!\"\n \
set VAR STRING\t\tAssigns a value to shell memory\n \
print VAR\t\tDisplays the STRING assigned to VAR\n \
source SCRIPT.TXT\tExecutes the file SCRIPT.TXT\n \
echo TOKEN\t\tDisplays TOKEN or variable value\n \
my_touch FILE\t\tCreates an empty file\n \
my_cd DIR\t\tChanges the current directory\n \
my_ls\t\t\tLists files in the current directory\n \
my_mkdir DIR\t\tCreates a new directory\n \
run COMMAND ARGS\tExecutes an external command\n ";
    printf("%s\n", help_string);
    return 0;
}

// Exits the shell with a goodbye message
int quit() {
    printf("Bye!\n");
    exit(0);
}

// Stores a variable-value pair in shell memory
int set(char *var, char *value) {
    mem_set_value(var, value);
    return 0;
}

// Retrieves and prints the value of a variable from shell memory
int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

// Echoes the token or variable value if token starts with $
int echo_cmd(char *token) {
    if (token == NULL) {
        printf("\n");
        return 0;
    }

    // Handle variable expansion: $VARNAME gets replaced with its value
    if (token[0] == '$') {
        char *varname = token + 1;
        if (varname[0] == '\0') {
            printf("\n");
            return 0;
        }

        char *val = mem_get_value(varname);

        // If variable doesn't exist, just print newline
        // mem_get_value returns a string that needs freeing only if variable exists
        if (strcmp(val, "Variable does not exist") == 0) {
            printf("\n");
        } else {
            printf("%s\n", val);
            free(val);  // Free the dynamically allocated value string
        }
        return 0;
    }

    // Not a variable, just print the token as-is
    printf("%s\n", token);
    return 0;
}

// Creates an empty file (only alphanumeric filenames allowed)
int my_touch(char *filename) {
    if (filename == NULL || filename[0] == '\0') {
        return badcommandMyTouch();
    }

    // Only allow alphanumeric filenames for security
    if (!is_alnum_string(filename)) {
        return badcommandMyTouch();
    }

    // Open in append mode to create file if it doesn't exist (doesn't overwrite existing files)
    FILE *f = fopen(filename, "a");
    if (f == NULL) {
        return badcommandMyTouch();
    }
    fclose(f);  // Just creating the file, no need to write anything
    return 0;
}

// Changes the current working directory
int my_cd(char *dirname) {
    if (dirname == NULL || dirname[0] == '\0') {
        return badcommandMyCd();
    }

    if (!is_alnum_string(dirname)) {
        return badcommandMyCd();
    }

    if (!is_alnum_string(dirname)) {
        return badcommandMyCd();
    }

    // chdir returns 0 on success, non-zero on failure (e.g., directory doesn't exist)
    if (chdir(dirname) != 0) {
        return badcommandMyCd();
    }
    return 0;
}

// Lists all files in current directory, sorted alphabetically
int my_ls() {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        return badcommand();
    }

    // Dynamic array to store filenames (starts with capacity 16, doubles when full)
    size_t cap = 16;
    size_t n = 0;
    char **names = malloc(cap * sizeof(char *));
    if (names == NULL) {
        closedir(dir);
        return badcommand();
    }

    // Read all directory entries into the array
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Expand array if we've run out of space (double the capacity)
        if (n == cap) {
            cap *= 2;
            char **tmp = realloc(names, cap * sizeof(char *));
            if (tmp == NULL) {
                // Clean up all allocated strings and array before returning
                for (size_t i = 0; i < n; i++) free(names[i]);
                free(names);
                closedir(dir);
                return badcommand();
            }
            names = tmp;
        }

        // Duplicate the filename string (we need our own copy since entry is reused)
        names[n] = strdup(entry->d_name);
        if (names[n] == NULL) {
            // Clean up all allocated strings and array before returning
            for (size_t i = 0; i < n; i++) free(names[i]);
            free(names);
            closedir(dir);
            return badcommand();
        }
        n++;
    }

    closedir(dir);

    // Sort filenames alphabetically before printing
    qsort(names, n, sizeof(char *), cmp_strings);

    // Print sorted list and clean up memory
    for (size_t i = 0; i < n; i++) {
        printf("%s\n", names[i]);
        free(names[i]);
    }
    free(names);

    return 0;
}

// Creates a new directory with full permissions (0777 = rwxrwxrwx)
int my_mkdir(char *dirname) {
    if (dirname == NULL || dirname[0] == '\0') {
        return badcommandMyMkdir();
    }

    // If argument is a variable: my_mkdir $VAR
    // mkdir returns 0 on success, non-zero on failure (e.g., directory already exists)
    char *name_to_make = dirname;

    // Handle $VAR case
    if (dirname[0] == '$') {
        char *var = dirname + 1;
        if (var[0] == '\0') return badcommandMyMkdir();

        char *val = mem_get_value(var);

        // Missing variable
        if (strcmp(val, "Variable does not exist") == 0) {
            return badcommandMyMkdir();
        }
        

        // Must be exactly one alphanumeric token
        if (!is_alnum_string(val)) {
            free(val);
            return badcommandMyMkdir();
        }

        int rc = mkdir(val, 0777);
        free(val);

        if (rc != 0) return badcommandMyMkdir();
        return 0;
    }

    // Non-$ case: enforce alphanumeric directory name
    if (!is_alnum_string(dirname)) {
        return badcommandMyMkdir();
    }

    if (mkdir(dirname, 0777) != 0) {
        return badcommandMyMkdir();
    }

    return 0;
}



int source(char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    // Read and execute each line until EOF
    // Note: errCode keeps the last error code from any command in the script
    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput(line);  // Execute the command
        memset(line, 0, sizeof(line));  // Clear the buffer for next read

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);
    return errCode;  // Return last error code encountered (or 0 if all succeeded)
}


// Executes an external command by forking and using execvp
int run_cmd(char *command_args[], int args_size) {
    pid_t pid;
    int status;
    
    if (args_size < 2) {
        return badcommand();
    }
    
    pid = fork();  // Create a new process
    
    if (pid < 0) {
        // Fork failed
        return badcommand();
    } else if (pid == 0) {
        // Child process: prepare args (skip "run" command, add NULL terminator)
        // Using variable length array (VLA) - size known at runtime
        char *exec_args[args_size];
        
        for (int i = 1; i < args_size; i++) {
            exec_args[i - 1] = command_args[i];  // Skip command_args[0] which is "run"
        }
        exec_args[args_size - 1] = NULL;  // execvp requires NULL-terminated array
        
        // Replace this process with the external command
        // If execvp fails (command not found), exit with error code
        execvp(exec_args[0], exec_args);
        exit(1);  // Only reached if execvp fails
    } else {
        // Parent process: wait for child to finish executing
        wait(&status);
        return 0;
    }
}
