#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // chdir
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 3;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
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

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);

int echo_cmd(char *token);
int my_touch(char *filename);
int my_cd(char *dirname);

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
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

    } else {
        return badcommand();
    }
}

int help() {
    char help_string[] =
        "COMMAND\t\t\tDESCRIPTION\n \
help\t\t\tDisplays all the commands\n \
quit\t\t\tExits / terminates the shell with “Bye!”\n \
set VAR STRING\t\tAssigns a value to shell memory\n \
print VAR\t\tDisplays the STRING assigned to VAR\n \
source SCRIPT.TXT\tExecutes the file SCRIPT.TXT\n \
echo TOKEN\t\tDisplays TOKEN or variable value\n \
my_touch FILE\t\tCreates an empty file\n \
my_cd DIR\t\tChanges the current directory\n ";
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
    // One token only:
    // - if token starts with '$', echo prints the variable's value, or a blank line if not found
    // - otherwise echo prints the token
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

        // starter mem_get_value returns this literal when not found
        if (strcmp(val, "Variable does not exist") == 0) {
            printf("\n");
        } else {
            printf("%s\n", val);
            free(val); // mem_get_value strdup()s successful lookups
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

    FILE *f = fopen(filename, "a"); // create if doesn't exist
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

    if (chdir(dirname) != 0) {
        return badcommandMyCd();
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
        errCode = parseInput(line); // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);
    return errCode;
}
