#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>

#define SHMSIZE 128
#define SHM_R 0400
#define SHM_W 0200
#define BUFFER_SIZE 10 // Circular buffer size

// Structure for the shared memory
struct shm_struct {
    int buffer[BUFFER_SIZE]; // Circular buffer for storing items
    unsigned next_in;        // Index for producer
    unsigned next_out;       // Index for consumer
    unsigned count;          // Number of items in the buffer
};

// Function to generate random delay
void random_delay(double min_seconds, double max_seconds) {
    double delay = min_seconds + ((double)rand() / RAND_MAX) * (max_seconds - min_seconds);
    usleep((int)(delay * 1e6)); // Convert seconds to microseconds
}

int main(int argc, char **argv) {
    volatile struct shm_struct *shmp = NULL;
    char *addr = NULL;
    pid_t pid = -1;
    int shmid = -1;

    // Seed random number generator
    srand(time(NULL));

    // Allocate shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(struct shm_struct), IPC_CREAT | SHM_R | SHM_W);
    shmp = (struct shm_struct *)shmat(shmid, addr, 0);

    // Initialize shared memory
    shmp->next_in = 0;
    shmp->next_out = 0;
    shmp->count = 0;

    pid = fork();
    if (pid != 0) {
        /* Parent process: Producer */
        for (int var1 = 1; var1 <= 100; ++var1) {
            // Wait until there's at least one empty slot
            while (shmp->count == BUFFER_SIZE); // Busy wait

            // Write to the buffer
            shmp->buffer[shmp->next_in] = var1;
            printf("Producer: Placed %d in buffer[%u]\n", var1, shmp->next_in);
            shmp->next_in = (shmp->next_in + 1) % BUFFER_SIZE; // Move to next slot
            shmp->count++; // Increment count of items

            fflush(stdout);

            // Wait a random time between 0.1s - 0.5s
            random_delay(0.1, 0.5);
        }

        // Detach and clean up shared memory
        shmdt(addr);
        shmctl(shmid, IPC_RMID, NULL);
    } else {
        /* Child process: Consumer */
        for (int var2 = 1; var2 <= 100; ++var2) {
            // Wait until there's at least one filled slot
            while (shmp->count == 0); // Busy wait

            // Read from the buffer
            int value = shmp->buffer[shmp->next_out];
            printf("Consumer: Retrieved %d from buffer[%u]\n", value, shmp->next_out);
            shmp->next_out = (shmp->next_out + 1) % BUFFER_SIZE; // Move to next slot
            shmp->count--; // Decrement count of items

            fflush(stdout);

            // Wait a random time between 0.2s - 2.0s
            random_delay(0.2, 2.0);
        }

        // Detach shared memory
        shmdt(addr);
    }

    return 0;
}
