#include "shell.h"

// Global variables imported from main.c and shell.c
extern int last_exit_status; 
extern void add_job(pid_t pid, const char* cmd_line, int is_background);
extern char* shell_name;

// --- Feature-5: Redirection and Pipe Helpers ---

void setup_redirection(command_t* cmd) {
    int fd;

    if (cmd->input_file != NULL) {
        fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror("myshell: open input file error");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("myshell: dup2 input error");
            exit(EXIT_FAILURE);
        }
        close(fd);
    }

    if (cmd->output_file != NULL) {
        fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("myshell: open output file error");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("myshell: dup2 output error");
            exit(EXIT_FAILURE);
        }
        close(fd);
    }
}

void execute_simple_command(command_t* cmd) {
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0) {
        // Child process
        signal(SIGINT, SIG_DFL);
        
        setup_redirection(cmd);

        if (execvp(cmd->arglist[0], cmd->arglist) == -1) {
            perror("myshell: execution error");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("myshell: fork error");
    } else {
        // Parent process
        if (cmd->is_background) {
            // Feature-6 & 9: Background execution
            printf("[%d] %d\n", 1, pid);
            
            char cmd_line[1024] = {0};
            for (int i = 0; cmd->arglist[i] != NULL; i++) {
                strcat(cmd_line, cmd->arglist[i]);
                strcat(cmd_line, " ");
            }
            add_job(pid, cmd_line, 1);
            
        } else {
            // Foreground execution (Blocking wait)
            do {
                wpid = waitpid(pid, &status, 0);
            } while (wpid == -1 && errno == EINTR);

            // Feature-7: Update the global exit status
            if (WIFEXITED(status)) {
                last_exit_status = WEXITSTATUS(status);
            } else {
                last_exit_status = 1; 
            }
        }
    }
}

void execute_piped_command(command_t* cmd) {
    int fd_in = 0;
    command_t* current_cmd = cmd;
    pid_t last_pid = -1;

    while (current_cmd != NULL) {
        int pipefd[2];

        if (current_cmd->next_pipe != NULL) {
            if (pipe(pipefd) == -1) {
                perror("myshell: pipe error");
                return;
            }
        }

        pid_t pid = fork();

        if (pid == 0) {
            // Child Process
            signal(SIGINT, SIG_DFL); 

            // 1. Handle Input
            if (fd_in != 0) {
                if (dup2(fd_in, STDIN_FILENO) < 0) { perror("myshell: dup2 input pipe error"); exit(EXIT_FAILURE); }
                close(fd_in);
            }

            // 2. Handle Output
            if (current_cmd->next_pipe != NULL) {
                close(pipefd[0]);
                if (dup2(pipefd[1], STDOUT_FILENO) < 0) { perror("myshell: dup2 output pipe error"); exit(EXIT_FAILURE); }
                close(pipefd[1]);
            }

            // 3. Handle Redirection
            setup_redirection(current_cmd);

            // 4. Execute
            if (execvp(current_cmd->arglist[0], current_cmd->arglist) == -1) {
                perror("myshell: execution error");
                exit(EXIT_FAILURE);
            }

        } else if (pid < 0) {
            perror("myshell: fork error");
            return;
        } else {
            // Parent Process
            last_pid = pid;

            if (current_cmd->next_pipe != NULL) {
                close(pipefd[1]);
            }
            
            if (fd_in != 0) {
                close(fd_in);
            }

            if (current_cmd->next_pipe != NULL) {
                fd_in = pipefd[0];
            } else {
                // If this is the LAST command in the pipe chain:
                if (cmd->is_background) {
                    // Feature-6 & 9: Background execution
                    printf("[%d] %d\n", 1, last_pid);
                    
                    char cmd_line[1024] = {0};
                    command_t* runner = cmd;
                    while (runner != NULL) {
                        for (int i = 0; runner->arglist[i] != NULL; i++) {
                            strcat(cmd_line, runner->arglist[i]);
                            strcat(cmd_line, " ");
                        }
                        if (runner->next_pipe != NULL) strcat(cmd_line, "| ");
                        runner = runner->next_pipe;
                    }
                    add_job(last_pid, cmd_line, 1);

                } else {
                    // Foreground execution (Blocking wait for the last PID)
                    int status;
                    do {
                        waitpid(last_pid, &status, 0);
                    } while (waitpid(last_pid, &status, 0) == -1 && errno == EINTR);
                    
                    // Feature-7: Update the global exit status
                    if (WIFEXITED(status)) {
                        last_exit_status = WEXITSTATUS(status);
                    } else {
                        last_exit_status = 1;
                    }
                }
            }
        }
        
        current_cmd = current_cmd->next_pipe;
    }
}


void execute_command(command_t* cmd) {
    if (cmd->next_pipe == NULL) {
        execute_simple_command(cmd);
    } else {
        execute_piped_command(cmd);
    }
}
