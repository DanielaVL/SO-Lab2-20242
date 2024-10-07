// #include <sys/types.h>
// #include <stdio.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <sys/time.h>

// arr[] vueltas = {"/bin/"}

// int main() {
//     pid_t pid;
//     struct timeval start_time, end_time;
//     long seconds, microseconds;
//     double elapsed;

//     gettimeofday(&start_time, NULL);

//     /* fork a child process */
//     pid = fork();
//     if (pid < 0) { /* error occurred */
//         fprintf(stderr, "Fork Failed");
//         return 1;
//     }
//     else if (pid == 0) { /* child process */

//         char* arr[] = {"ls", "-l", "-R", "-a", NULL};
//         for vuelta in vueltas:
//             path_vuelta = "/bin/" + arr[0]
//             "/bin/ls"
//             execv(path_vuelta, arr);
//     }
//     else { /* parent process */
//         /* parent will wait for the child to complete */
//         wait(NULL);

//         gettimeofday(&end_time, NULL);

//         seconds = end_time.tv_sec - start_time.tv_sec;
//         microseconds = end_time.tv_usec - start_time.tv_usec;

//         // Total elapsed time in milliseconds (optional)
//         elapsed = seconds + microseconds*1e-6;

//         printf("Elapsed time: %ld seconds and %ld microseconds\n", seconds, microseconds);
//         printf("Total elapsed time: %.6f seconds\n", elapsed);

//         printf("Child Complete");

//     }
//     return 0;
// }