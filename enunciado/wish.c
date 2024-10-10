#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_COMMANDS 100
#define MAX_ARGS 100

// Define the default search path
char *paths[100];
int path_count = 1;

void initialize_paths() {
    paths[0] = "/bin";
}

// Function to handle the built-in 'exit' command
void handle_exit() {
    exit(0);
}

// Function to handle the built-in 'cd' command
void handle_cd(char **args, int arg_count) {
    if (arg_count != 2) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }
    if (chdir(args[1]) != 0) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

// Function to handle the built-in 'path' command
void handle_path(char **args, int arg_count) {
    //path_count = 0;
    for (int i = 1; i < arg_count; i++) {
        paths[path_count++] = strdup(args[i]);
    }
}

// Function to execute a single command with possible redirection
void execute_single_command(char **args, int arg_count, char *output_file) {
    int has_redirection = 0;
    char *cmd_args[MAX_ARGS];
    int cmd_arg_count = 0;
    char *invalid = NULL;

    // Separate command and arguments from redirection
    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], ">") == 0) {
            has_redirection = 1;
            if (i + 1 < arg_count) {
                output_file = args[i + 1];
                invalid = args[i + 2];
                if (invalid != NULL) {
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    return;
                }
                break;
            } else {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                return;
            }
        }
        cmd_args[cmd_arg_count++] = args[i];
    }
    cmd_args[cmd_arg_count] = NULL;

    if (cmd_arg_count == 0) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }


    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (has_redirection && output_file != NULL) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        // Try all paths
        for (int i = 0; i < path_count; i++) {
            char executable_path[256];
            snprintf(executable_path, sizeof(executable_path), "%s/%s", paths[i], cmd_args[0]);
            if (access(executable_path, X_OK) == 0) {    
                execv(executable_path, cmd_args);
            }
            
        }
        
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    } else if (pid < 0) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

// Function to parse and execute multiple commands in parallel
void execute_parallel_commands(char *line) {
    char *commands[MAX_COMMANDS];
    int cmd_count = 0;
    pid_t pids[MAX_COMMANDS];

    // Split the line into separate commands
    char *cmd = strtok(line, "&");
    while (cmd != NULL && cmd_count < MAX_COMMANDS) {
        // Trim leading and trailing whitespace
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        int len = strlen(cmd);
        while (len > 0 && (cmd[len-1] == ' ' || cmd[len-1] == '\t')) {
            cmd[--len] = '\0';
        }
        
        if (len > 0) {
            commands[cmd_count++] = cmd;
        }
        cmd = strtok(NULL, "&");
    }

    // Execute each command
    for (int i = 0; i < cmd_count; i++) {
        char *args[MAX_ARGS];
        int arg_count = 0;
        char *output_file = NULL;

        // Parse the command into arguments
        char *token = strtok(commands[i], " \t");
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t");
        }
        args[arg_count] = NULL;

        if (arg_count > 0) {
            // Handle built-in commands
            if (strcmp(args[0], "exit") == 0) {
                if (arg_count != 1) {
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    continue;
                }
                handle_exit();
            } else if (strcmp(args[0], "cd") == 0) {
                handle_cd(args, arg_count);
            } else if (strcmp(args[0], "path") == 0) {
                handle_path(args, arg_count);
            } else {
                execute_single_command(args, arg_count, output_file);
            }
        }
    }

    // Wait for all child processes to complete
    while (wait(NULL) > 0);
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    initialize_paths();

    FILE *input = stdin;
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    } else if (argc > 2) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    while (1) {
        if (input == stdin) {
            printf("wish> ");
        }

        nread = getline(&line, &len, input);
        if (nread == -1) {
            break;
        }

        // Remove the trailing newline character
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        execute_parallel_commands(line);
    }

    free(line);
    if (input != stdin) {
        fclose(input);
    }
    return 0;
}
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <sys/types.h>
// #include <sys/stat.h>

// // Define the default search path
// char *paths[100];
// int path_count = 1;

// void initialize_paths() {
//     paths[0] = "/bin";
// }

// // Function to handle the built-in 'exit' command
// void handle_exit() {
//     exit(0);
// }

// // Function to handle the built-in 'cd' command
// void handle_cd(char **args, int arg_count) {
//     if (arg_count != 2) {
//         fprintf(stderr, "An error has occurred\n");
//         return;
//     }
//     if (chdir(args[1]) != 0) {
//         perror("cd failed");
//     }
// }

// // Function to handle the built-in 'path' command
// void handle_path(char **args, int arg_count) {
//     //path_count = 0;
//     for (int i = 1; i < arg_count; i++) {
//         paths[path_count++] = args[i];
//     }
// }

// // Function to execute shell scripts
// void execute_shell_script(char *script_path) {
//     if (fork() == 0) {
//         execl("/bin/bash", "bash", script_path, NULL);
//         perror("execl failed");
//         exit(1);
//     } else {
//         wait(NULL);
//     }
// }

// // Function to execute external commands
// void execute_command(char **args) {
//     struct stat sb;
//     if (stat(args[0], &sb) == 0 && sb.st_mode & S_IXUSR) {
//         // If the command is a shell script
//         execute_shell_script(args[0]);
//         return;
//     }
//     if (path_count == 0) {
//         fprintf(stderr, "An error has occurred\n");
//         return;
//     }
//     for (int i = 0; i < path_count; i++) {
//         char executable_path[256];
//         snprintf(executable_path, sizeof(executable_path), "%s/%s", paths[i], args[0]);
//         if (access(executable_path, X_OK) == 0) {
//             // Found the executable, now execute it
//             if (fork() == 0) {
//                 execv(executable_path, args);
//                 perror("execv failed");
//                 exit(1);
//             } else {
//                 wait(NULL);
//                 return;
//             }
//         }
//     }
//     fprintf(stderr, "An error has occurred\n");
// }

// int main(int argc, char *argv[]) {
//     char *line = NULL;
//     size_t len = 0;
//     ssize_t nread;

//     initialize_paths();

//     FILE *input = stdin;
//     if (argc == 2) {
//         input = fopen(argv[1], "r");
//         if (!input) {
//             perror("Failed to open batch file");
//             exit(1);
//         }
//     } else if (argc > 2) {
//         fprintf(stderr, "Usage: wish [batch_file]\n");
//         exit(1);
//     }

//     while (1) {
//         if (input == stdin) {
//             printf("wish> ");
//         }

//         nread = getline(&line, &len, input);
//         if (nread == -1) {
//             break;
//         }

//         // Remove the trailing newline character
//         if (line[nread - 1] == '\n') {
//             line[nread - 1] = '\0';
//         }

//         // Parse the input line
//         char *args[100];
//         int arg_count = 0;
//         char *token = strtok(line, " ");
//         while (token != NULL) {
//             args[arg_count++] = token;
//             token = strtok(NULL, " ");
//         }
//         args[arg_count] = NULL;

//         if (arg_count == 0) {
//             continue; // Empty command
//         }

//         // Handle built-in commands
//         if (strcmp(args[0], "exit") == 0) {
//             if (arg_count != 1) {
//                 fprintf(stderr, "An error has occurred\n");
//                 continue;
//             }
//             handle_exit();
//         } else if (strcmp(args[0], "cd") == 0) {
//             handle_cd(args, arg_count);
//         } else if (strcmp(args[0], "path") == 0) {
//             handle_path(args, arg_count);
//         } else {
//             execute_command(args);
//         }
//     }

//     free(line);
//     if (input != stdin) {
//         fclose(input);
//     }
//     return 0;
// }