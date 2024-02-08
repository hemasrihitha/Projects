#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100

void tokenizeInput(char *input, char **tokens) {
    char *token = strtok(input, " \t\n");
    int i = 0;

    while (token != NULL) {
        tokens[i++] = token;
        token = strtok(NULL, " \t\n");
    }

    tokens[i] = NULL;
}

void executeCommand(char **tokens, int background) {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        if (background) {
            setsid();  // Run background process in a new session
        }

        // Check for output redirection
        int outputRedirectIndex = -1;
        for (int i = 0; tokens[i] != NULL; ++i) {
            if (strcmp(tokens[i], ">") == 0) {
                outputRedirectIndex = i;
                break;
            }
        }

        if (outputRedirectIndex != -1) {
            // Output redirection
            char *outputFile = tokens[outputRedirectIndex + 1];
            int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd == -1) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);

            // Remove the '>' and the output file from the tokens
            tokens[outputRedirectIndex] = NULL;
            tokens[outputRedirectIndex + 1] = NULL;
        }

        execvp(tokens[0], tokens);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        if (!background) {
            waitpid(pid, NULL, 0);  // Wait for the child to finish
        }
    } else {
        // Fork failed
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
}

void sigchldHandler(int signum) {
    // Handle SIGCHLD (child process terminated)
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_TOKENS];

    signal(SIGCHLD, sigchldHandler);

    while (1) {
        printf("shell> ");
        fgets(input, sizeof(input), stdin);

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            exit(EXIT_SUCCESS);
        } else if (strcmp(input, "") == 0) {
            continue;  // Ignore blank lines
        }

        int background = 0;

        // Check if the command should run in the background
        if (input[strlen(input) - 1] == '&') {
            background = 1;
            input[strlen(input) - 1] = '\0';
        }

        tokenizeInput(input, tokens);

        // Execute the command
        executeCommand(tokens, background);
    }

    return 0;
}
