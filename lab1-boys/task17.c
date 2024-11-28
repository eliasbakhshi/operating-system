#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    int *pages;           // Array to store pages in memory
    int front;            // Points to the oldest page in memory (FIFO)
    int rear;             // Points to the most recently added page
    int capacity;         // Maximum number of physical pages
    int size;             // Current number of pages in memory
} Queue;

// Function to initialize the queue with a given capacity
Queue* createQueue(int capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
    queue->pages = (int*)malloc(capacity * sizeof(int));
    return queue;
}

// Function to check if a page is in the queue (i.e., in memory)
bool isPageInMemory(Queue* queue, int page) {
    for (int i = 0; i < queue->size; i++) {
        if (queue->pages[(queue->front + i) % queue->capacity] == page) {
            return true;
        }
    }
    return false;
}

// Function to add a page to the queue (FIFO page replacement)
void enqueuePage(Queue* queue, int page) {
    if (queue->size == queue->capacity) {
        // Memory is full, remove the oldest page
        queue->front = (queue->front + 1) % queue->capacity;
        queue->size--;
    }
    // Add the new page
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->pages[queue->rear] = page;
    queue->size++;
}

// Function to free allocated memory for the queue
void freeQueue(Queue* queue) {
    free(queue->pages);
    free(queue);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s no_phys_pages page_size filename\n", argv[0]);
        return 1;
    }

    int no_phys_pages = atoi(argv[1]);
    int page_size = atoi(argv[2]);
    const char *filename = argv[3];

    printf("No physical pages = %d, page size = %d\n", no_phys_pages, page_size);

    // Initialize page queue for FIFO replacement
    Queue *pageQueue = createQueue(no_phys_pages);
    int pageFaults = 0;

    // Open the memory trace file
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        freeQueue(pageQueue);
        return 1;
    }

    // Read each memory reference from the file
    unsigned int address;
    while (fscanf(file, "%u", &address) == 1) {
        // Calculate the page number
        int page = address / page_size;

        // Check if the page is already in memory
        if (!isPageInMemory(pageQueue, page)) {
            // Page fault occurs
            pageFaults++;
            enqueuePage(pageQueue, page);
        }
    }

    fclose(file);
    freeQueue(pageQueue);

    // Output the result
    printf("Result: %d page faults\n", pageFaults);
    return 0;
}
