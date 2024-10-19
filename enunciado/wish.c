#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_ARGS 64  // Número máximo de argumentos que se espera que un comando tenga

char *searchPath[MAX_ARGS] = {"/bin", NULL};  // La ruta de búsqueda inicial es /bin

void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void changeDirectory(char *args[]) {
    if (args[1] == NULL || args[2] != NULL) {  // Debe tener solo un argumento
        printError();
    } else if (chdir(args[1]) != 0) {
        printError();
    }
}

void setPath(char *args[]) {
    // Limpiar la ruta actual
    for (int i = 0; searchPath[i] != NULL; i++) {
        searchPath[i] = NULL;
    }

    // Establecer la nueva ruta a partir de los argumentos
    for (int i = 1; args[i] != NULL && i < MAX_ARGS; i++) {
        searchPath[i - 1] = args[i];
    }
}

void executeCommand(char *args[]) {
    int redirect_index = -1;
    
    // Verificar si no hay comando antes del operador '>'
    if (args[0] != NULL && strcmp(args[0], ">") == 0) {
        printError();  // No se permite redirección sin un comando antes
        return;
    }

    // Buscar si hay un operador '>'
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            redirect_index = i;
            break;
        }
    }

    // Si hay redirección, verificar la validez del comando
    if (redirect_index != -1) {
        if (args[redirect_index + 1] == NULL || args[redirect_index + 2] != NULL) {
            // No hay archivo de salida o hay más de uno
            printError();
            return;
        }

        // Abrir el archivo para redirigir la salida
        FILE *output_file = fopen(args[redirect_index + 1], "w");
        if (output_file == NULL) {
            printError();
            return;
        }

        // Redirigir stdout y stderr
        int fd = fileno(output_file);  // Obtener el descriptor de archivo
        pid_t pid = fork();
        if (pid == 0) {  // Proceso hijo
            // Redirigir stdout y stderr al archivo
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            fclose(output_file);  // Cerrar el archivo en el hijo después de redirigir

            // Eliminar los argumentos de redirección de los argumentos de comando
            args[redirect_index] = NULL; // Termina la lista de argumentos aquí

            // Ejecutar el comando
            for (int i = 0; searchPath[i] != NULL; i++) {
                char command[1024];
                snprintf(command, sizeof(command), "%s/%s", searchPath[i], args[0]);

                if (access(command, X_OK) == 0) {
                    execv(command, args);
                    printError();  // Si execv falla
                    exit(1);
                }
            }

            // Si no encuentra el comando, imprime error
            printError();
            exit(1);
        } else if (pid > 0) {  // Proceso padre
            // Cerrar el archivo en el proceso padre
            fclose(output_file);
            wait(NULL);  // Esperar al hijo
            return;
        } else {
            printError();
            return;
        }
    } else {
        // Si no hay redirección, ejecutar el comando normalmente
        for (int i = 0; searchPath[i] != NULL; i++) {
            char command[1024];
            snprintf(command, sizeof(command), "%s/%s", searchPath[i], args[0]);

            if (access(command, X_OK) == 0) {
                pid_t pid = fork();
                if (pid == 0) {  // Proceso hijo
                    execv(command, args);
                    printError();  // Si execv falla
                    exit(1);
                } else if (pid > 0) {  // Proceso padre
                    wait(NULL);  // Esperar al hijo
                    return;
                } else {
                    printError();
                    return;
                }
            }
        }
        printError();  // No se encontró el ejecutable
    }
}

void executeParallelCommands(char *line) {
    char *commands[MAX_ARGS];
    int command_count = 0;
    pid_t pids[MAX_ARGS];

    // Dividir la línea por el carácter '&'
    char *token = strsep(&line, "&");
    while (token != NULL && command_count < MAX_ARGS - 1) {
        if (*token != '\0') {  // Ignorar comandos vacíos
            commands[command_count++] = token;
        }
        token = strsep(&line, "&");
    }
    commands[command_count] = NULL;

    // Ejecutar cada comando en paralelo
    for (int i = 0; i < command_count; i++) {
        char *args[MAX_ARGS];
        char *command = commands[i];

        // Tokenizar el comando
        int arg_count = 0;
        char *arg_token = strsep(&command, " ");
        while (arg_token != NULL && arg_count < MAX_ARGS - 1) {
            if (*arg_token != '\0') {  // Ignorar tokens vacíos
                args[arg_count++] = arg_token;
            }
            arg_token = strsep(&command, " ");
        }
        args[arg_count] = NULL;

        // Si el comando no es vacío, ejecutar
        if (args[0] != NULL) {
            pid_t pid = fork();
            if (pid == 0) {  // Proceso hijo
                executeCommand(args);
                exit(0);  // Asegurarse de salir del proceso hijo
            } else if (pid > 0) {  // Proceso padre
                pids[i] = pid;
            } else {
                printError();
            }
        }
    }

    // Esperar a que todos los procesos hijos terminen
    for (int i = 0; i < command_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

void processInput(FILE *input) {
    char *line = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];

    while (1) {
        if (input == stdin) {
            printf("wish> ");  // Mostrar prompt solo en modo interactivo
        }

        ssize_t nread = getline(&line, &len, input);
        if (nread == -1) {
            free(line);  // Liberar memoria
            return;  // EOF o error
        }

        line[strcspn(line, "\n")] = 0;  // Eliminar salto de línea

        // Eliminar espacios en blanco al principio de la línea
        while (isspace(*line)) {
            line++;
        }

        if (strchr(line, '&') != NULL) {
            // Si la línea contiene '&', ejecutar comandos en paralelo
            executeParallelCommands(line);
        } else {
            // Procesar comando normal si no hay '&'
            int i = 0;
            char *token;
            while ((token = strsep(&line, " ")) != NULL && i < MAX_ARGS - 1) {
                if (*token != '\0') {  // Ignorar tokens vacíos (delimitadores consecutivos)
                    args[i++] = token;
                }
            }
            args[i] = NULL;

            if (args[0] == NULL) {  // Comando vacío
                continue;
            }

            // Comandos internos
            if (strcmp(args[0], "exit") == 0) {
                if (args[1] != NULL) {
                    printError();
                } else {
                    free(line);  // Liberar memoria antes de salir
                    exit(0);
                }
            } else if (strcmp(args[0], "cd") == 0) {
                changeDirectory(args);
            } else if (strcmp(args[0], "path") == 0) {
                setPath(args);
            } else {
                executeCommand(args);  // Comando externo
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        processInput(stdin);  // Modo interactivo
    } else if (argc == 2) {
        FILE *batchFile = fopen(argv[1], "r");
        if (batchFile == NULL) {
            printError();
            exit(1);
        }
        processInput(batchFile);
        fclose(batchFile);
    } else {
        printError();
        exit(1);
    }
    return 0;
}