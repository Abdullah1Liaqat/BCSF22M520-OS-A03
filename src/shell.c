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
