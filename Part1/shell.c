#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>              // isatty, fileno
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput(char ui[]);

int main(int argc, char *argv[]) {
    printf("Shell version 1.5 created Dec 2025\n");

    char prompt = '$';
    char userInput[MAX_USER_INPUT];
    int errorCode = 0;

    // init user input
    memset(userInput, 0, sizeof(userInput));

    // init shell memory
    mem_init();

    // interactive if stdin is a terminal, batch if stdin is redirected
    int interactive = isatty(fileno(stdin));

    while (1) {
        if (interactive) {
            printf("%c ", prompt);
            fflush(stdout);
        }

        // Read a line; if EOF, exit (this fixes the batch infinite loop)
        if (fgets(userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            break;
        }

        errorCode = parseInput(userInput);

        // quit command signals termination (interpreter typically returns -1)
        if (errorCode == -1) {
            break;
        }

        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

static int isWordEnding(char c) {
    // Treat ';' as a delimiter so we can split chained commands
    return c == '\0' || c == '\n' || c == ' ' || c == ';' || c == '\t' || c == '\r';
}

static void freeWords(char **words, int count) {
    for (int i = 0; i < count; i++) {
        free(words[i]);
    }
}

static int runOneCommand(char *cmd) {
    char tmp[200];
    char *words[100];
    int w = 0;

    // Skip leading whitespace
    int ix = 0;
    while ((cmd[ix] == ' ' || cmd[ix] == '\t' || cmd[ix] == '\r') && cmd[ix] != '\0') {
        ix++;
    }

    // Empty command (e.g. ";;" or leading ";") is allowed, just do nothing
    if (cmd[ix] == '\0' || cmd[ix] == '\n') {
        return 0;
    }

    // Tokenize by whitespace (not by ';' here, we already split on ';')
    while (cmd[ix] != '\0' && cmd[ix] != '\n') {
        // Skip whitespace between tokens
        while ((cmd[ix] == ' ' || cmd[ix] == '\t' || cmd[ix] == '\r') && cmd[ix] != '\0') {
            ix++;
        }
        if (cmd[ix] == '\0' || cmd[ix] == '\n') break;

        // Extract one token
        int wordlen = 0;
        while (cmd[ix] != '\0' && cmd[ix] != '\n' && !isWordEnding(cmd[ix]) && wordlen < (int)sizeof(tmp) - 1) {
            tmp[wordlen++] = cmd[ix++];
        }
        tmp[wordlen] = '\0';

        if (wordlen > 0) {
            words[w++] = strdup(tmp);
        }

        // If we hit a delimiter that's not whitespace, move past it
        if (cmd[ix] != '\0' && cmd[ix] != '\n' && cmd[ix] != ' ' && cmd[ix] != '\t' && cmd[ix] != '\r') {
            ix++;
        }

        if (w >= 100) break;
    }

    int errorCode = interpreter(words, w);
    freeWords(words, w);
    return errorCode;
}

int parseInput(char inp[]) {
    // Implement chaining: split the line on ';' and run commands in order
    // Max total input length is already bounded by MAX_USER_INPUT (1000 in the assignment)
    // We will run at most 10 chained commands (as per assignment)
    char line[MAX_USER_INPUT];
    strncpy(line, inp, MAX_USER_INPUT - 1);
    line[MAX_USER_INPUT - 1] = '\0';

    int commandsRun = 0;

    char *saveptr = NULL;
    char *segment = strtok_r(line, ";", &saveptr);

    while (segment != NULL && commandsRun < 10) {
        int ec = runOneCommand(segment);
        if (ec == -1) return -1; // quit
        commandsRun++;
        segment = strtok_r(NULL, ";", &saveptr);
    }

    return 0;
}
