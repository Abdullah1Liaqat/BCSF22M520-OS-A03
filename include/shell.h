#ifndef SHELL_H
#define SHELL_H

// Required includes based on features implemented up to Feature-6
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h> // Added for Feature-6 whitespace trimming

// Feature-3: Command History size
#define HISTORY_SIZE 20

// Feature-4: Readline includes
#include <readline/readline.h>
#include <readline/history.h>

// Global variable for history
extern char* custom_history_list[HISTORY_SIZE];
extern int history_count;

// Struct to hold parsed command data (extended for Feature-6)
typedef struct command_t {
    char** arglist;
    char* input_file;    // For '<' redirection
    char* output_file;   // For '>' redirection
    struct command_t* next_pipe; // For '|' piping
    
    // Feature-6 additions
    int is_background;      // Flag for '&' background execution
    struct command_t* next_chain; // For ';' command chaining
} command_t;

// --- Function Prototypes ---

// main.c
void setup_environment();
void cleanup_resources();
void sigchld_handler(int sig); // For zombie prevention (Feature-6)
void sigint_handler(int sig);  // To ignore Ctrl+C (Feature-6)

// shell.c
char** my_completion(const char* text, int start, int end);
command_t* parse_command(char* line);
command_t* parse_chain_segment(char* segment); // Helper for parsing
void free_command(command_t* cmd);
int handle_builtin(command_t* cmd);
void init_history();
void add_to_history_list(const char* cmd);
int reexecute_history(command_t* cmd);

// execute.c
void execute_command(command_t* cmd);
void execute_simple_command(command_t* cmd);
void execute_piped_command(command_t* cmd);
void setup_redirection(command_t* cmd);

#endif // SHELL_H

