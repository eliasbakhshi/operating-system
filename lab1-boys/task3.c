// Include necessary libraries for input/output, process control, inter-process communication, and time
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

// Define constants for the size of shared memory, read and write permissions, and buffer size
#define SHMSIZE sizeof(struct shm_struct)
#define SHM_R 0400
#define SHM_W 0200
#define BUFFER_SIZE 10

// Define a structure for shared memory
struct shm_struct {
    int buffer[BUFFER_SIZE]; // Buffer for storing data
    int next_to_write; // Index to write next
    int next_to_read; // Index to read next
    sem_t empty_slots;  // Semaphore for empty slots in the buffer
    sem_t filled_slots; // Semaphore for filled slots in the buffer
};

// Function to sleep for a random duration
void random_sleep(double min_seconds, double max_seconds) {
    double fraction = (double)rand() / RAND_MAX; // Generate a random fraction
    double duration = min_seconds + fraction * (max_seconds - min_seconds); // Calculate duration
    usleep((useconds_t)(duration * 1000000)); // Sleep for the calculated duration
}

// Main function
int main(int argc, char **argv) {
    srand(time(NULL)); // Seed for random number generation

    volatile struct shm_struct *shmp = NULL; // Pointer to shared memory
    char *addr = NULL; // Address for shared memory
    pid_t pid = -1; // Process ID
    int var1 = 0, var2 = 0, shmid = -1; // Variables for data and shared memory ID
    struct shmid_ds shm_buf; // Buffer for shared memory data

    // Allocate a chunk of shared memory
    shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | SHM_R | SHM_W); // Get shared memory ID
    shmp = (struct shm_struct *)shmat(shmid, addr, 0); // Attach shared memory
    shmp->next_to_write = 0; // Initialize next_to_write
    shmp->next_to_read = 0; // Initialize next_to_read

    // Initialize semaphores
    sem_init(&shmp->empty_slots, 1, BUFFER_SIZE); // Initialize semaphore for empty slots
    sem_init(&shmp->filled_slots, 1, 0); // Initialize semaphore for filled slots

    pid = fork(); // Create a new process
    if (pid != 0) {
        // Parent process (Producer)
        while (var1 < 100) {
            sem_wait(&shmp->empty_slots); // Wait for an empty slot

            // Produce an item
            var1++; // Increment var1
            printf("Sending %d\n", var1); fflush(stdout); // Print the produced item
            shmp->buffer[shmp->next_to_write] = var1; // Write the item to the buffer
            shmp->next_to_write = (shmp->next_to_write + 1) % BUFFER_SIZE; // Update the index to write next

            sem_post(&shmp->filled_slots); // Signal a filled slot

            // Sleep for a random time between 0.1s - 0.5s
            random_sleep(0.1, 0.5);
        }
    } else {
        // Child process (Consumer)
        while (var2 < 100) {
            sem_wait(&shmp->filled_slots); // Wait for a filled slot

            // Consume an item
            var2 = shmp->buffer[shmp->next_to_read]; // Read the item from the buffer
            printf("Received %d\n", var2); fflush(stdout); // Print the received item
            shmp->next_to_read = (shmp->next_to_read + 1) % BUFFER_SIZE; // Update the index to read next

            sem_post(&shmp->empty_slots); // Signal an empty slot

            // Sleep for a random time between 0.2s - 2s
            random_sleep(0.2, 2.0);
        }
    }

    // Cleanup: Destroy semaphores, detach and remove the shared memory
    sem_destroy(&shmp->empty_slots); // Destroy semaphore for empty slots
    sem_destroy(&shmp->filled_slots); // Destroy semaphore for filled slots
    shmdt(addr); // Detach shared memory
    shmctl(shmid, IPC_RMID, &shm_buf); // Remove shared memory

    return 0; // Return 0 to indicate successful execution
}