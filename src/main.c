#include "shell.h"

// Feature-3 Globals
char* custom_history_list[HISTORY_SIZE] = {NULL};
int history_count = 0;

// Feature-6: Handler for SIGCHLD (Zombie Prevention)
void sigchld_handler(int sig) {
    // Collect status of all terminated children without blocking
    // WNOHANG prevents the shell from pausing.
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Feature-6: Handler for SIGINT (Ignore Ctrl+C)
void sigint_handler(int sig) {
    // Print a new line and clear the Readline buffer to reset the prompt
    printf("\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

void setup_environment() {
    // Feature-4 FIX: Set the custom completion function
    rl_attempted_completion_function = my_completion;
    init_history();

    // Feature-6: Set up signal handlers
    signal(SIGCHLD, sigchld_handler); // To reap zombie processes
    signal(SIGINT, sigint_handler);   // To ignore Ctrl+C in the shell process itself
}

void cleanup_resources() {
    for (int i = 0; i < history_count; i++) {
        if (custom_history_list[i] != NULL) {
            free(custom_history_list[i]);
        }
    }
}

int main(int argc, char** argv) {
    char* line;
    command_t* head_cmd;
    
    setup_environment();

    while (1) {
        line = readline("myshell> ");

        if (line == NULL) { // EOF (Ctrl+D)
            printf("exit\n");
            break;
        }

        if (line[0] != '\0') {
            add_history(line);
        }
        
        // Feature-3: Handle !n re-execution
        if (line[0] == '!') {
            command_t* temp_cmd = parse_command(line);
            if (temp_cmd != NULL) {
                reexecute_history(temp_cmd);
                free_command(temp_cmd);
                free(line);
                continue;
            } else {
                free(line);
                continue;
            }
        }
        
        // Feature-3: Store in our custom history list (for !n)
        if (line[0] != '\0') {
            add_to_history_list(line);
        }

        // Parse the line into a chained command structure
        head_cmd = parse_command(line);
        free(line);

        if (head_cmd == NULL) {
            continue;
        }

        // Feature-6: Process the command chain (separated by ';')
        command_t* current_chain = head_cmd;
        while (current_chain != NULL) {
            command_t* next = current_chain->next_chain;
            current_chain->next_chain = NULL; // Decouple for clean single-command freeing

            if (current_chain->arglist != NULL && current_chain->arglist[0] != NULL) {
                if (!handle_builtin(current_chain)) {
                    execute_command(current_chain);
                }
            }

            // Free the command struct that was just executed
            free_command(current_chain);
            current_chain = next;
        }
    }

    cleanup_resources();
    return 0;
}
