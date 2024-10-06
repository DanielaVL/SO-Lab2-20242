#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

int main() {
    pid_t pid;
    struct timeval start_time, end_time;
    long seconds, microseconds;
    double elapsed;

    gettimeofday(&start_time, NULL);

    /* fork a child process */
    pid = fork();
    if (pid < 0) { /* error occurred */
        fprintf(stderr, "Fork Failed");
        return 1;
    }
    else if (pid == 0) { /* child process */
        char *args[] = {"/bin/ls", NULL}; 
        execv("/bin/ls", args);
        // Si execv falla
        perror("execv");
        return 1;
    }
    else { /* parent process */
        /* parent will wait for the child to complete */
        wait(NULL);

        gettimeofday(&end_time, NULL);

        // Calcular la diferencia en segundos y microsegundos
        seconds = end_time.tv_sec - start_time.tv_sec;
        microseconds = end_time.tv_usec - start_time.tv_usec;

        // Ajuste en caso de que microsegundos sea negativo
        if (microseconds < 0) {
            seconds -= 1;
            microseconds += 1000000;
        }

        // Calcular el tiempo total transcurrido en segundos
        elapsed = seconds + microseconds * 1e-6;

        printf("Elapsed time: %ld seconds and %ld microseconds\n", seconds, microseconds);
        printf("Total elapsed time: %.6f seconds\n", elapsed);
    }
    return 0;
}