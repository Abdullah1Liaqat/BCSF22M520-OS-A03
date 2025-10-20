#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
int main() {
    // Enable TAB filename completion
    rl_bind_key('\t', rl_complete);

    while (1) {
        char *cmdline = readline(PROMPT);     // Readline prints prompt & reads input
        if (cmdline == NULL) break;            // Ctrl+D

        if (strlen(cmdline) == 0) {            // Empty line
            free(cmdline);
            continue;
        }

        add_history(cmdline);                  // Readline’s own history list

        char **arglist = tokenize(cmdline);
        if (arglist != NULL) {
            if (handle_builtin(arglist)) {
                for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
                free(arglist);
                free(cmdline);
                continue;
            }

            execute(arglist);

            for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}

