#include "shell.h"

// ... (Existing built-in functions: shell_exit, shell_cd, shell_help, shell_jobs)
// ... (Existing history functions: init_history, add_to_history_list, shell_history, reexecute_history)
// ... (Existing handle_builtin function)

// --- Feature-4: Tab Completion Implementation ---
char** my_completion(const char* text, int start, int end) {
    return rl_completion_matches(text, rl_filename_completion_function);
}

// --- Parsing and Tokenization (Feature-6 ready) ---

command_t* create_command() {
    command_t* cmd = (command_t*)malloc(sizeof(command_t));
    cmd->arglist = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->next_pipe = NULL;
    cmd->is_background = 0; // Feature-6 Init
    cmd->next_chain = NULL; // Feature-6 Init
    return cmd;
}

// Helper to free a command_t structure (only its pipe chain, as chain is decoupled in main)
void free_command(command_t* cmd) {
    if (cmd == NULL) return;
    
    // Recursively free the pipe chain
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

// Helper to parse a single chain segment (handles pipes, redirection, and '&')
command_t* parse_chain_segment(char* segment) {
    if (segment == NULL || segment[0] == '\0') return NULL;

    // Feature-6: Check for background '&'
    int is_background = 0;
    size_t len = strlen(segment);
    if (len > 0 && segment[len - 1] == '&') {
        is_background = 1;
        segment[len - 1] = '\0'; // Null-terminate before the '&'
        // Trim any preceding whitespace
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

        // Pipe/Redirection logic from Feature-5
        char* token_segment = strdup(pipe_segment);
        char* token;
        char* saveptr_token;
        int arg_count = 0;
        
        // Pass 1: Count tokens for arglist size
        char* count_copy = strdup(token_segment);
        char* temp_token;
        char* saveptr_count;
        
        temp_token = strtok_r(count_copy, " \t\r\n\a", &saveptr_count);
        while (temp_token != NULL) {
            if (strcmp(temp_token, "<") != 0 && strcmp(temp_token, ">") != 0) {
                arg_count++;
            } else {
                // Skip the filename token too
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
                current_cmd->arglist[i++] = strdup(token);
            }
            token = strtok_r(NULL, " \t\r\n\a", &saveptr_token);
        }
        current_cmd->arglist[i] = NULL;
        
        free(token_segment);
        
        // Move to the next pipe segment
        pipe_segment = strtok_r(NULL, "|", &saveptr_pipe);

        // If this is the *last* command in the pipe chain, set the background flag
        if (pipe_segment == NULL && is_background) {
            current_cmd->is_background = 1;
        }
    }
    
    free(segment_copy);
    return head;
}


command_t* parse_command(char* line) {
    if (line == NULL || line[0] == '\0') return NULL;
    
    command_t* head_chain = NULL;
    command_t* current_chain_link = NULL;
    
    char* line_copy = strdup(line);
    char* segment;
    char* saveptr_chain;
    
    // Feature-6: Split by ';' for command chaining
    segment = strtok_r(line_copy, ";", &saveptr_chain);
    
    while (segment != NULL) {
        // Trim whitespace from segment
        size_t len = strlen(segment);
        while (len > 0 && isspace((unsigned char)segment[len-1])) {
            segment[--len] = '\0';
        }
        char *trimmed_segment = segment;
        while (*trimmed_segment && isspace((unsigned char)*trimmed_segment)) {
            trimmed_segment++;
        }
        
        if (*trimmed_segment) {
            // Parse the segment for pipes, redirection, and '&'
            command_t* new_cmd_head = parse_chain_segment(trimmed_segment);
            
            if (new_cmd_head != NULL) {
                // Link the chain
                if (head_chain == NULL) {
                    head_chain = new_cmd_head;
                    current_chain_link = new_cmd_head;
                } else {
                    current_chain_link->next_chain = new_cmd_head;
                    current_chain_link = new_cmd_head;
                }
                
                // Advance to the end of the pipe chain to find the actual chain link
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
