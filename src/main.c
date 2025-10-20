#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    rl_bind_key('\t', rl_complete);   // Enable TAB auto-completion

    while (1) {
        // readline() handles prompt display and editing
        char *cmdline = readline(PROMPT);
        if (cmdline == NULL) break;   // Exit on Ctrl+D

        // Skip empty input lines
        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        // Add non-empty command to history
        add_history(cmdline);

        // Tokenize the command
        char **arglist = tokenize(cmdline);
        if (arglist != NULL) {
            // Check and handle built-in commands first
            if (handle_builtin(arglist)) {
                for (int i = 0; arglist[i] != NULL; i++)
                    free(arglist[i]);
                free(arglist);
                free(cmdline);
                continue;
            }

            // Execute external command
            execute(arglist);

            // Free memory
            for (int i = 0; arglist[i] != NULL; i++)
                free(arglist[i]);
            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
