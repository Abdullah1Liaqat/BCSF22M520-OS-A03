#include "shell.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>#include <stdio.h>

/*
 * handle_builtin()
 * Checks if command is a built-in and 
executes it.
 * Returns 1 if handled inside the shell, 0 otherwise.
 */
int handle_builtin(char **args)
{
    if (args[0] == NULL)
        return 0;   // empty line

    // exit
    if (strcmp(args[0], "exit") == 0)
    {
        printf("Exiting shell...\n");
        exit(0);
    }

    // cd
    if (strcmp(args[0], "cd") == 0)
    {
        if (args[1] == NULL)
            fprintf(stderr, "cd: expected argument\n");
        else if (chdir(args[1]) != 0)
            perror("cd");
        return 1;
    }

    // help
    if (strcmp(args[0], "help") == 0)
    {
        printf("\n---- FCIT Shell Help ----\n");
        printf("Built-in commands:\n");
        printf("  cd <dir>   Change directory\n");
        printf("  exit       Exit the shell\n");
        printf("  help       Show this help message\n");
        printf("  jobs       Show background jobs (placeholder)\n");
        printf("--------------------------\n\n");
        return 1;
    }

    // jobs
    if (strcmp(args[0], "jobs") == 0)
    {
        printf("No background jobs yet (feature coming soon)\n");
        return 1;
    }
    if (strcmp(args[0], "history") == 0) {
        print_history();
        return 1;
    }

    if (args[0][0] == '!') {
        return handle_history_recall(args);
    }


    return 0;   // not a built-in
}

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace
        
        if (*cp == '\0') break; // Line was only whitespace

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
// ---------- Command History ----------


static char *history[HISTORY_SIZE];
static int history_count = 0;

void add_history(const char *cmdline) {
    if (cmdline == NULL || strlen(cmdline) == 0) return;

    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmdline);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
            history[i - 1] = history[i];
        history[HISTORY_SIZE - 1] = strdup(cmdline);
    }
}

void print_history(void) {
    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i]);
    }
}

// Handle !n command
int handle_history_recall(char **args) {
    if (args[0][0] != '!') return 0;

    int index = atoi(args[0] + 1) - 1;
    if (index < 0 || index >= history_count) {
        printf("No such command in history.\n");
        return 1;
    }

    printf("Re-executing: %s\n", history[index]);
    char *cmdline = strdup(history[index]);
    char **new_args = tokenize(cmdline);
    if (new_args != NULL) {
        execute(new_args);
        for (int i = 0; new_args[i] != NULL; i++) free(new_args[i]);
        free(new_args);
    }
    free(cmdline);
    return 1;
}
