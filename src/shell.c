#include "shell.h"

// ... (Existing built-in functions)
// ... (Existing history functions)
// ... (Existing my_completion function)

// Global variable imported from main.c for Feature-7 logic
extern int last_exit_status; 

// ... (Existing create_command and free_command helpers)
// ... (Existing parse_chain_segment helper)

// --- Feature-7: 'if-then-else-fi' Block Processor ---
command_t* parse_if_block(char* block) {
    if (block == NULL || block[0] == '\0') return NULL;
    
    // We expect the block to be a multi-line string containing the full logic.
    // E.g., "if\n cmd1 \nthen\n cmd2 \nelse\n cmd3 \nfi"
    
    command_t* head_chain = NULL;
    command_t* current_chain_link = NULL;
    
    char* block_copy = strdup(block);
    char* saveptr_block;
    char* token;
    
    // Split the block by newlines and spaces to find keywords
    token = strtok_r(block_copy, " \t\r\n", &saveptr_block);
    
    // States for the parser
    enum { IF_WAIT, THEN_WAIT, ELSE_WAIT, IF_CMD, THEN_CMD, ELSE_CMD, END_BLOCK } state = IF_WAIT;
    
    // Buffer to hold the command string for the current block (IF, THEN, or ELSE)
    char cmd_buffer[2048] = {0};
    
    // Flag to store which commands to actually execute
    int execute_then = 0; // Assume THEN block is not executed by default
    
    // Collect the IF condition command and evaluate it first
    while (token != NULL) {
        if (strcmp(token, "if") == 0) {
            state = IF_CMD;
        } else if (strcmp(token, "then") == 0) {
            state = THEN_WAIT;
            // 1. EVALUATE THE IF CONDITION (stored in cmd_buffer)
            if (cmd_buffer[0] != '\0') {
                command_t* condition_cmd = parse_command(cmd_buffer);
                
                // Execute the condition command in foreground and get status
                if (condition_cmd != NULL) {
                    if (!handle_builtin(condition_cmd)) {
                        execute_command(condition_cmd);
                    }
                    free_command(condition_cmd);
                }
                
                // Set the execution flag based on global last_exit_status
                execute_then = (last_exit_status == 0); 
            }
            // Clear buffer and move to collect THEN commands
            cmd_buffer[0] = '\0';
            state = THEN_CMD;

        } else if (strcmp(token, "else") == 0) {
            // Move to collect ELSE commands
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
                    
                    // The result must be a single chain linked via next_chain
                    if (final_cmd_chain) {
                        if (head_chain == NULL) {
                            head_chain = final_cmd_chain;
                            current_chain_link = head_chain;
                        } else {
                            // Find the end of the existing chain
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
            break; // Done with parsing
            
        } else if (state == IF_CMD || state == THEN_CMD || state == ELSE_CMD) {
            // 3. Collect commands for the current block
            
            // If the state just transitioned from IF_CMD/THEN_CMD/ELSE_CMD
            // and we encounter a command token that is NOT one of the keywords, 
            // process the currently collected buffer first.
            if (state != IF_CMD && state != THEN_CMD && state != ELSE_CMD && cmd_buffer[0] != '\0') {
                
                command_t* cmd_chain = parse_command(cmd_buffer);
                
                // The result must be a single chain linked via next_chain
                if (cmd_chain) {
                    // Check conditional execution
                    int execute_current = 0;
                    if (state == THEN_CMD) execute_current = execute_then;
                    else if (state == ELSE_CMD) execute_current = !execute_then;

                    if (execute_current) {
                         if (head_chain == NULL) {
                            head_chain = cmd_chain;
                            current_chain_link = head_chain;
                        } else {
                            // Find the end of the existing chain
                            command_t* runner = head_chain;
                            while (runner->next_chain != NULL) {
                                runner = runner->next_chain;
                            }
                            runner->next_chain = cmd_chain;
                            current_chain_link = cmd_chain;
                        }
                    } else {
                        free_command(cmd_chain); // Free the chain if not executed
                    }
                }
                cmd_buffer[0] = '\0';
            }
            
            // Append the token to the current command buffer
            if (cmd_buffer[0] != '\0') {
                strcat(cmd_buffer, " ");
            }
            strcat(cmd_buffer, token);
        }
        
        token = strtok_r(NULL, " \t\r\n", &saveptr_block);
    }
    
    free(block_copy);
    
    // Return the resulting chain of commands that SHOULD be executed
    return head_chain;
}


// --- Standard Parsing Function (Modified for Feature 7 integration) ---

command_t* parse_command(char* line) {
    if (line == NULL || line[0] == '\0') return NULL;
    
    // --- Feature 7 Integration ---
    if (strstr(line, "\n") != NULL && strstr(line, "if") != NULL && strstr(line, "fi") != NULL) {
        // If the line contains newlines and the if/fi keywords, treat it as a block.
        // This is only called from main.c's multi-line parser.
        return parse_if_block(line);
    }
    // --- End Feature 7 Integration ---
    
    // ... (rest of the existing Feature-6 parsing logic)
    // ... (It is assumed that the rest of parse_command is the existing Feature-6 code)
    
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
            command_t* new_cmd_head = parse_chain_segment(trimmed_segment);
            
            if (new_cmd_head != NULL) {
                // Link the chain
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
                
                // Advance to the end of the pipe chain
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
