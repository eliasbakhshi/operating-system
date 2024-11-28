#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    pid_t pid1, pid2; // Variables to store the return values of fork()

    // First fork: creates the first child process
    pid1 = fork();

    if (pid1 == 0) {
        // First child process: prints "A"
        for (int i = 0; i < 100; i++) {
            printf("A = %d, ", i);
        }
        printf("\nFirst child (A) => Own pid: %d, Parent's pid: %d\n", getpid(), getppid());
        return 0; // Exit first child process
    }

    // Second fork: creates the second child process
    pid2 = fork();

    if (pid2 == 0) {
        // Second child process: prints "C"
        for (int i = 0; i < 100; i++) {
            printf("C = %d, ", i);
        }
        printf("\nSecond child (C) => Own pid: %d, Parent's pid: %d\n", getpid(), getppid());
        return 0; // Exit second child process
    }

    // Parent process: prints "B" and the PIDs of the child processes
    if (pid1 > 0 && pid2 > 0) {
        // Parent prints "B"
        for (int i = 0; i < 100; i++) {
            printf("B = %d, ", i);
        }
        printf("\nParent => Own pid: %d\n", getpid());
        printf("Parent => First child's pid: %d\n", pid1);
        printf("Parent => Second child's pid: %d\n", pid2);
    }

    return 0;
}
