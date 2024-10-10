#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64  // Número máximo de argumentos que se espera que un comando tenga

// Variables globales
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

        // Redirigir stdout
        dup2(fileno(output_file), STDOUT_FILENO);
        fclose(output_file); // Cerrar el archivo después de redirigir

        // Eliminar los argumentos de redirección de los argumentos de comando
        args[redirect_index] = NULL; // Termina la lista de argumentos aquí
    }

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

void processInput(FILE *input) {
    char *line = NULL;  // Puntero para la línea
    size_t len = 0;     // Tamaño del buffer
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

        // Tokenizar la entrada usando strsep()
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