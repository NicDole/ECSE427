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

int MAX_ARGS_SIZE = 3;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command when file doesnt exist
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int badcommandMyCd() {
    printf("Bad command: my_cd\n");
    return 1;
}

int badcommandMyTouch() {
    printf("Bad command: my_touch\n");
    return 1;
}

int badcommandMyMkdir() {
    printf("Bad command: my_mkdir\n");
    return 1;
}

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

static int cmp_strings(const void *a, const void *b) {
    const char *sa = *(const char * const *)a;
    const char *sb = *(const char * const *)b;
    return strcmp(sa, sb);
}

// Takes parsed command arguments and routes to the right handler
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1) {
        return badcommand();
    }

    // Remove newlines from arguments
    for (i = 0; i < args_size; i++) {
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }
    
    // Run command can have variable arguments, others cant
    if (args_size > MAX_ARGS_SIZE) {
        if (strcmp(command_args[0], "run") != 0) {
            return badcommand();
        }
    }

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

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

int echo_cmd(char *token) {
    if (token == NULL) {
        printf("\n");
        return 0;
    }

    if (token[0] == '$') {
        char *varname = token + 1;
        if (varname[0] == '\0') {
            printf("\n");
            return 0;
        }

        char *val = mem_get_value(varname);

        if (strcmp(val, "Variable does not exist") == 0) {
            printf("\n");
        } else {
            printf("%s\n", val);
            free(val);
        }
        return 0;
    }

    printf("%s\n", token);
    return 0;
}

int my_touch(char *filename) {
    if (filename == NULL || filename[0] == '\0') {
        return badcommandMyTouch();
    }

    if (!is_alnum_string(filename)) {
        return badcommandMyTouch();
    }

    FILE *f = fopen(filename, "a");
    if (f == NULL) {
        return badcommandMyTouch();
    }
    fclose(f);
    return 0;
}

int my_cd(char *dirname) {
    if (dirname == NULL || dirname[0] == '\0') {
        return badcommandMyCd();
    }

    if (!is_alnum_string(dirname)) {
        return badcommandMyCd();
    }

    if (chdir(dirname) != 0) {
        return badcommandMyCd();
    }
    return 0;
}

int my_ls() {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        return badcommand();
    }

    size_t cap = 16;
    size_t n = 0;
    char **names = malloc(cap * sizeof(char *));
    if (names == NULL) {
        closedir(dir);
        return badcommand();
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (n == cap) {
            cap *= 2;
            char **tmp = realloc(names, cap * sizeof(char *));
            if (tmp == NULL) {
                for (size_t i = 0; i < n; i++) free(names[i]);
                free(names);
                closedir(dir);
                return badcommand();
            }
            names = tmp;
        }

        names[n] = strdup(entry->d_name);
        if (names[n] == NULL) {
            for (size_t i = 0; i < n; i++) free(names[i]);
            free(names);
            closedir(dir);
            return badcommand();
        }
        n++;
    }

    closedir(dir);

    qsort(names, n, sizeof(char *), cmp_strings);

    for (size_t i = 0; i < n; i++) {
        printf("%s\n", names[i]);
        free(names[i]);
    }
    free(names);

    return 0;
}

int my_mkdir(char *dirname) {
    if (dirname == NULL || dirname[0] == '\0') {
        return badcommandMyMkdir();
    }

    char *name_to_make = dirname;

    // Handle $VAR case
    if (dirname[0] == '$') {
        char *var = dirname + 1;
        if (var[0] == '\0') return badcommandMyMkdir();

        char *val = mem_get_value(var);

        // Your mem_get_value returns a newly-allocated string for existing vars
        // but returns the literal "Variable does not exist" for missing.
        if (strcmp(val, "Variable does not exist") == 0) {
            return badcommandMyMkdir();
        }

        // Must be exactly one alphanumeric token
        if (!is_alnum_string(val)) {
            free(val);
            return badcommandMyMkdir();
        }

        name_to_make = val;

        int rc = mkdir(name_to_make, 0777);
        free(val);

        if (rc != 0) return badcommandMyMkdir();
        return 0;
    }

    // Non-$ case: enforce alphanumeric directory name (recommended for tests)
    if (!is_alnum_string(name_to_make)) {
        return badcommandMyMkdir();
    }

    if (mkdir(name_to_make, 0777) != 0) {
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

    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput(line);
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);
    return errCode;
}

int run_cmd(char *command_args[], int args_size) {
    pid_t pid;
    int status;
    
    if (args_size < 2) {
        return badcommand();
    }
    
    pid = fork();
    
    if (pid < 0) {
        return badcommand();
    } else if (pid == 0) {
        char *exec_args[args_size];
        
        for (int i = 1; i < args_size; i++) {
            exec_args[i - 1] = command_args[i];
        }
        exec_args[args_size - 1] = NULL;
        
        execvp(exec_args[0], exec_args);
        exit(1);
    } else {
        wait(&status);
        return 0;
    }
}
