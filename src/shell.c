#include "shell.h"

// --- Feature 8: Shell Variable Structures and Globals ---
typedef struct shell_var_t {
    char* name;
    char* value;
    struct shell_var_t* next;
} shell_var_t;

// Static head for the shell variable list (local to this file)
static shell_var_t* var_list_head = NULL;

// --- Feature 9: Job Control Structures and Globals ---
typedef struct job_t {
    pid_t pid;
    char* cmd_line;
    int job_id;
    char* status; // E.g., "Running", "Done", "Stopped"
    struct job_t* next;
} job_t;

// Static head for the job list (local to this file)
static job_t* job_list_head = NULL;
static int next_job_id = 1;

// Global variable imported from main.c for Feature-7 logic
extern int last_exit_status; 

// --- Feature-2: Built-in Commands Implementation ---

const char* built_in_cmds[] = {"exit", "cd", "help", "jobs", "history", "set"};

void shell_exit(command_t* cmd) { exit(0); }

void shell_cd(command_t* cmd) {
    char* dir = cmd->arglist[1] ? cmd->arglist[1] : getenv("HOME");
    if (chdir(dir) != 0) {
        perror("myshell: cd error");
    }
}

void shell_help(command_t* cmd) {
    printf("\nMy Simple Shell (Built-in Commands):\n");
    printf("  exit                - Terminates the shell.\n");
    printf("  cd <directory>      - Changes the current working directory.\n");
    printf("  help                - Displays this help message.\n");
    printf("  jobs                - Lists active background jobs (Feature-9).\n");
    printf("  history             - Lists the command history.\n");
    printf("  set                 - Lists all shell variables (Feature-8).\n");
    printf("  VAR=VALUE           - Sets a shell variable (Feature-8).\n");
    printf("\nExternal commands are executed via fork/exec.\n");
}

void shell_jobs(command_t* cmd) {
    // Feature-9: Lists active jobs
    job_t* current = job_list_head;
    if (current == NULL) {
        printf("No active jobs.\n");
        return;
    }
    printf("Active Jobs:\n");
    while (current != NULL) {
        printf("[%d] %s\t\t%s\n", current->job_id, current->status, current->cmd_line);
        current = current->next;
    }
}

// --- Feature-8: Shell Variable Functions (set/get/display) ---

void set_shell_var(const char* name, const char* value) {
    shell_var_t* current = var_list_head;
    // Check if variable already exists
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            free(current->value);
            current->value = strdup(value);
            return;
        }
        current = current->next;
    }

    // Create a new variable
    shell_var_t* new_var = (shell_var_t*)malloc(sizeof(shell_var_t));
    new_var->name = strdup(name);
    new_var->value = strdup(value);
    new_var->next = var_list_head;
    var_list_head = new_var;
}

char* get_shell_var(const char* name) {
    shell_var_t* current = var_list_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL; // Not found
}

void shell_set(command_t* cmd) {
    // Feature-8: Lists all shell variables
    shell_var_t* current = var_list_head;
    if (current == NULL) {
        printf("No shell variables defined.\n");
        return;
    }
    while (current != NULL) {
        printf("%s=%s\n", current->name, current->value);
        current = current->next;
    }
}

// --- Feature-3: History Implementation ---

void init_history() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        custom_history_list[i] = NULL;
    }
    // Set initial environment variables (optional but good practice)
    set_shell_var("SHELL", "/bin/myshell"); 
    set_shell_var("VERSION", "v7+");
}

void add_to_history_list(const char* cmd_line) {
    // ... (Existing implementation)
    if (cmd_line == NULL || cmd_line[0] == '\0') return;

    if (history_count == HISTORY_SIZE) {
        free(custom_history_list[0]);
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            custom_history_list[i] = custom_history_list[i + 1];
        }
        custom_history_list[HISTORY_SIZE - 1] = strdup(cmd_line);
    } else {
        custom_history_list[history_count++] = strdup(cmd_line);
    }
}

void shell_history(command_t* cmd) {
    // ... (Existing implementation)
    for (int i = 0; i < history_count; i++) {
        printf("%4d  %s\n", i + 1, custom_history_list[i]);
    }
}

int reexecute_history(command_t* cmd) {
    // ... (Existing implementation)
    if (cmd->arglist == NULL || cmd->arglist[0] == NULL || cmd->arglist[0][0] != '!') return 0;
    
    char* token = cmd->arglist[0];
    int n = atoi(token + 1);
    
    if (n < 1 || n > history_count) {
        fprintf(stderr, "myshell: event not found: %s\n", token);
        return 1;
    }

    char* re_cmd_line = custom_history_list[n - 1];
    printf("%s\n", re_cmd_line);
    
    command_t* re_cmd = parse_command(re_cmd_line);
    if (re_cmd != NULL) {
        command_t* current_chain = re_cmd;
        while (current_chain != NULL) {
            command_t* next = current_chain->next_chain;
            current_chain->next_chain = NULL;
            if (current_chain->arglist != NULL && current_chain->arglist[0] != NULL) {
                if (!handle_builtin(current_chain)) {
                    execute_command(current_chain);
                }
            }
            free_command(current_chain);
            current_chain = next;
        }
    }
    return 1;
}

// --- Feature-9: Job Management Functions ---

void add_job(pid_t pid, const char* cmd_line, int is_background) {
    if (!is_background) return; // Only track background jobs

    job_t* new_job = (job_t*)malloc(sizeof(job_t));
    new_job->pid = pid;
    new_job->cmd_line = strdup(cmd_line);
    new_job->job_id = next_job_id++;
    new_job->status = strdup("Running"); 
    new_job->next = job_list_head;
    job_list_head = new_job;
}

void cleanup_job_list(void) {
    // Note: The SIGCHLD handler in main.c handles status updates. 
    // This simple cleanup function is primarily for the shell exit.
    job_t *current = job_list_head;
    job_t *next = NULL;
    while(current != NULL) {
        next = current->next;
        free(current->cmd_line);
        free(current->status);
        free(current);
        current = next;
    }
    job_list_head = NULL;
}

// --- Built-in Command Dispatch (Feature 8 Fix) ---

int handle_builtin(command_t* cmd) {
    if (cmd == NULL || cmd->arglist == NULL || cmd->arglist[0] == NULL || cmd->next_pipe != NULL) {
        return 0;
    }
    
    char* cmd_name = cmd->arglist[0];

    // Feature-8: Check for variable assignment: VAR=value
    char* equals = strchr(cmd_name, '=');
    if (equals != NULL) {
        *equals = '\0'; // Temporarily separate name and value
        
        // Basic validation: ensure name is not empty
        if (cmd_name[0] == '\0') {
            fprintf(stderr, "myshell: Syntax error: variable name missing.\n");
            return 1;
        }

        // Set the variable
        set_shell_var(cmd_name, equals + 1);

        // Restore the original string (optional, but good practice)
        *equals = '='; 
        return 1;
    }
    
    // Handle standard built-ins
    if (strcmp(cmd_name, "exit") == 0) {
        shell_exit(cmd);
        return 1;
    } else if (strcmp(cmd_name, "cd") == 0) {
        shell_cd(cmd);
        return 1;
    } else if (strcmp(cmd_name, "help") == 0) {
        shell_help(cmd);
        return 1;
    } else if (strcmp(cmd_name, "jobs") == 0) {
        shell_jobs(cmd);
        return 1;
    } else if (strcmp(cmd_name, "history") == 0) {
        shell_history(cmd);
        return 1;
    } else if (strcmp(cmd_name, "set") == 0) {
        shell_set(cmd); // Feature-8: Implemented set
        return 1;
    }
    
    return 0; // Not a built-in
}

// --- Feature-4: Tab Completion Implementation ---

char** my_completion(const char* text, int start, int end) {
    return rl_completion_matches(text, rl_filename_completion_function);
}

// --- Parsing and Tokenization (Feature-8 & 9 ready) ---

command_t* create_command() {
    command_t* cmd = (command_t*)malloc(sizeof(command_t));
    cmd->arglist = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->next_pipe = NULL;
    cmd->is_background = 0;
    cmd->next_chain = NULL;
    return cmd;
}

void free_command(command_t* cmd) {
    if (cmd == NULL) return;
    
    free_command(cmd->next_pipe); 

    if (cmd->arglist) {
        for (int i = 0; cmd->arglist[i] != NULL; i++) {
            free(cmd->arglist[i]);
        }
        free(cmd->arglist);
    }
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
    free(cmd);
}

// Helper to perform variable substitution on a single token
char* substitute_variables(const char* token) {
    if (token == NULL || token[0] == '\0') return strdup(token);
    
    char buffer[2048] = {0};
    const char* p = token;
    
    while (*p != '\0') {
        if (*p == '$') {
            p++; // Skip '$'
            const char* var_start = p;
            
            // Collect variable name (alphanumeric and underscore)
            while (isalnum((unsigned char)*p) || *p == '_') {
                p++;
            }
            
            // Handle special shell variables (e.g., $?)
            if (p == var_start && *p == '?') {
                char status_str[16];
                snprintf(status_str, sizeof(status_str), "%d", last_exit_status);
                strcat(buffer, status_str);
                p++;
                continue;
            }

            size_t name_len = p - var_start;
            if (name_len > 0) {
                char name[256];
                strncpy(name, var_start, name_len);
                name[name_len] = '\0';
                
                char* value = get_shell_var(name);
                if (value != NULL) {
                    strcat(buffer, value);
                }
            } else {
                // Lone '$' or malformed, treat as literal
                strcat(buffer, "$");
            }
        } else {
            // Append regular character
            size_t len = strlen(buffer);
            buffer[len] = *p;
            buffer[len + 1] = '\0';
            p++;
        }
    }
    
    return strdup(buffer);
}

command_t* parse_chain_segment(char* segment) {
    if (segment == NULL || segment[0] == '\0') return NULL;

    int is_background = 0;
    size_t len = strlen(segment);
    if (len > 0 && segment[len - 1] == '&') {
        is_background = 1;
        segment[len - 1] = '\0';
        while (len > 0 && isspace((unsigned char)segment[len-2])) {
            segment[len-2] = '\0';
            len--;
        }
    }

    command_t* head = create_command();
    command_t* current_cmd = head;
    
    char* pipe_segment;
    char* segment_copy = strdup(segment);
    char* saveptr_pipe;
    
    pipe_segment = strtok_r(segment_copy, "|", &saveptr_pipe);
    
    while (pipe_segment != NULL) {
        if (current_cmd != head) {
            current_cmd->next_pipe = create_command();
            current_cmd = current_cmd->next_pipe;
        }

        char* token_segment = strdup(pipe_segment);
        char* token;
        char* saveptr_token;
        int arg_count = 0;
        
        // Pass 1: Count tokens for arglist size (after substitution)
        char* count_copy = strdup(token_segment);
        char* temp_token;
        char* saveptr_count;
        
        temp_token = strtok_r(count_copy, " \t\r\n\a", &saveptr_count);
        while (temp_token != NULL) {
            if (strcmp(temp_token, "<") != 0 && strcmp(temp_token, ">") != 0) {
                // If the token is not a redirection operator, it's an argument
                arg_count++; 
            } else {
                // If it's redirection, skip the operator and the next token (filename)
                temp_token = strtok_r(NULL, " \t\r\n\a", &saveptr_count);
            }
            temp_token = strtok_r(NULL, " \t\r\n\a", &saveptr_count);
        }
        free(count_copy);

        current_cmd->arglist = (char**)calloc(arg_count + 1, sizeof(char*));
        int i = 0;
        
        // Pass 2: Populate arglist and redirection fields
        token = strtok_r(token_segment, " \t\r\n\a", &saveptr_token);
        while (token != NULL) {
            if (strcmp(token, "<") == 0) {
                token = strtok_r(NULL, " \t\r\n\a", &saveptr_token);
                if (token) current_cmd->input_file = strdup(token);
            } else if (strcmp(token, ">") == 0) {
                token = strtok_r(NULL, " \t\r\n\a", &saveptr_token);
                if (token) current_cmd->output_file = strdup(token);
            } else {
                // Feature-8: Perform substitution before storing the argument
                current_cmd->arglist[i++] = substitute_variables(token);
            }
            token = strtok_r(NULL, " \t\r\n\a", &saveptr_token);
        }
        current_cmd->arglist[i] = NULL;
        
        free(token_segment);
        
        pipe_segment = strtok_r(NULL, "|", &saveptr_pipe);

        if (pipe_segment == NULL && is_background) {
            current_cmd->is_background = 1;
        }
    }
    
    free(segment_copy);
    return head;
}

// ... (Existing parse_if_block function from Feature 7) ...
command_t* parse_if_block(char* block) {
    if (block == NULL || block[0] == '\0') return NULL;
    
    command_t* head_chain = NULL;
    command_t* current_chain_link = NULL;
    
    char* block_copy = strdup(block);
    char* saveptr_block;
    char* token;
    
    enum { IF_WAIT, THEN_WAIT, ELSE_WAIT, IF_CMD, THEN_CMD, ELSE_CMD, END_BLOCK } state = IF_WAIT;
    
    char cmd_buffer[2048] = {0};
    
    int execute_then = 0; 
    
    token = strtok_r(block_copy, " \t\r\n", &saveptr_block);
    
    while (token != NULL) {
        if (strcmp(token, "if") == 0) {
            state = IF_CMD;
        } else if (strcmp(token, "then") == 0) {
            state = THEN_WAIT;
            
            // 1. EVALUATE THE IF CONDITION (stored in cmd_buffer)
            if (cmd_buffer[0] != '\0') {
                command_t* condition_cmd = parse_command(cmd_buffer);
                
                if (condition_cmd != NULL) {
                    if (!handle_builtin(condition_cmd)) {
                        execute_command(condition_cmd);
                    }
                    free_command(condition_cmd);
                }
                
                execute_then = (last_exit_status == 0); 
            }
            cmd_buffer[0] = '\0';
            state = THEN_CMD;

        } else if (strcmp(token, "else") == 0) {
            state = ELSE_CMD;
            cmd_buffer[0] = '\0';

        } else if (strcmp(token, "fi") == 0) {
            // END OF BLOCK
            
            // 2. PROCESS THE FINAL BLOCK (either THEN or ELSE)
            if (cmd_buffer[0] != '\0') {
                int execute_final_block = 0;
                
                if (state == THEN_CMD) {
                    execute_final_block = execute_then;
                } else if (state == ELSE_CMD) {
                    execute_final_block = !execute_then;
                }
                
                if (execute_final_block) {
                    command_t* final_cmd_chain = parse_command(cmd_buffer);
                    
                    if (final_cmd_chain) {
                        if (head_chain == NULL) {
                            head_chain = final_cmd_chain;
                            current_chain_link = head_chain;
                        } else {
                            command_t* runner = head_chain;
                            while (runner->next_chain != NULL) {
                                runner = runner->next_chain;
                            }
                            runner->next_chain = final_cmd_chain;
                            current_chain_link = final_cmd_chain;
                        }
                    }
                }
            }
            cmd_buffer[0] = '\0';
            state = END_BLOCK;
            break; 
            
        } else if (state == IF_CMD || state == THEN_CMD || state == ELSE_CMD) {
            // 3. Collect commands for the current block
            if (cmd_buffer[0] != '\0') {
                strcat(cmd_buffer, " ");
            }
            strcat(cmd_buffer, token);
        }
        
        token = strtok_r(NULL, " \t\r\n", &saveptr_block);
    }
    
    free(block_copy);
    return head_chain;
}

// --- Standard Parsing Function (Feature 7 Integration) ---

command_t* parse_command(char* line) {
    if (line == NULL || line[0] == '\0') return NULL;
    
    if (strstr(line, "\n") != NULL && strstr(line, "if") != NULL && strstr(line, "fi") != NULL) {
        return parse_if_block(line);
    }
    
    command_t* head_chain = NULL;
    command_t* current_chain_link = NULL;
    
    char* line_copy = strdup(line);
    char* segment;
    char* saveptr_chain;
    
    segment = strtok_r(line_copy, ";", &saveptr_chain);
    
    while (segment != NULL) {
        size_t len = strlen(segment);
        while (len > 0 && isspace((unsigned char)segment[len-1])) {
            segment[--len] = '\0';
        }
        char *trimmed_segment = segment;
        while (*trimmed_segment && isspace((unsigned char)*trimmed_segment)) {
            trimmed_segment++;
        }
        
        if (*trimmed_segment) {
            command_t* new_cmd_head = parse_chain_segment(trimmed_segment);
            
            if (new_cmd_head != NULL) {
                if (head_chain == NULL) {
                    head_chain = new_cmd_head;
                    current_chain_link = new_cmd_head;
                } else {
                    command_t* runner = head_chain;
                    while (runner->next_chain != NULL) {
                        runner = runner->next_chain;
                    }
                    runner->next_chain = new_cmd_head;
                    current_chain_link = new_cmd_head;
                }
                
                while (current_chain_link->next_pipe != NULL) {
                    current_chain_link = current_chain_link->next_pipe;
                }
            }
        }
        segment = strtok_r(NULL, ";", &saveptr_chain);
    }
    
    free(line_copy);
    return head_chain;
}
