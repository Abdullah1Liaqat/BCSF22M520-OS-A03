#include "shell.h"

// ... (Existing setup_redirection function)

void execute_simple_command(command_t* cmd) {
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0) {
        // Child process
        signal(SIGINT, SIG_DFL); // Reset SIGINT handler to default for the child
        
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
            // Feature-6: Background execution (Non-blocking)
            printf("[1] %d\n", pid);
            // The SIGCHLD handler in main.c will reap the zombie later
        } else {
            // Foreground execution (Blocking wait)
            do {
                wpid = waitpid(pid, &status, 0);
            } while (wpid == -1 && errno == EINTR);
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
                    // Feature-6: Background execution
                    printf("[1] %d\n", last_pid);
                } else {
                    // Foreground execution (Blocking wait for the last PID)
                    int status;
                    do {
                        waitpid(last_pid, &status, 0);
                    } while (waitpid(last_pid, &status, 0) == -1 && errno == EINTR);
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
