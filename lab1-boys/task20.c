#include <stdio.h>                // Includes standard input/output functions.
#include <stdlib.h>               // Includes memory allocation and utility functions.
#include <stdbool.h>              // Enables usage of `true` and `false` in the code.

typedef struct Node {             // Defines a doubly linked list node structure.
    int page;                     // Stores the page number.
    struct Node* next;            // Pointer to the next node in the linked list.
    struct Node* prev;            // Pointer to the previous node in the linked list.
} Node;

typedef struct {                  // Defines a structure for the LRU Cache.
    Node* head;                   // Points to the most recently used node (head of the cache).
    Node* tail;                   // Points to the least recently used node (tail of the cache).
    int size;                     // Current number of nodes/pages in the cache.
    int capacity;                 // Maximum number of nodes/pages the cache can hold.
} LRUCache;

LRUCache* createCache(int capacity) { // Creates and initializes an LRU cache with given capacity.
    LRUCache* cache = (LRUCache*)malloc(sizeof(LRUCache)); // Allocates memory for the cache.
    cache->head = NULL;            // Initializes the cache as empty; `head` is set to NULL.
    cache->tail = NULL;            // Initializes the cache as empty; `tail` is set to NULL.
    cache->size = 0;               // Sets the current size of the cache to 0.
    cache->capacity = capacity;    // Stores the maximum capacity of the cache.
    return cache;                  // Returns the pointer to the newly created cache.
}

void moveToHead(LRUCache* cache, Node* node) { // Moves a node to the head of the cache.
    if (cache->head == node) return;           // If the node is already the head, do nothing.
    if (node->prev) node->prev->next = node->next; // Update the previous node to skip this node.
    if (node->next) node->next->prev = node->prev; // Update the next node to skip this node.
    if (cache->tail == node) cache->tail = node->prev; // If node was the tail, update the tail pointer.
    node->next = cache->head;             // Make the current `head` the next node of this node.
    node->prev = NULL;                    // As this node will become the head, no previous node exists.
    if (cache->head) cache->head->prev = node; // Update the old head’s previous pointer to this node.
    cache->head = node;                   // Set this node as the new head.
    if (!cache->tail) cache->tail = node; // If the cache was empty, set this node as the tail too.
}

void removeTail(LRUCache* cache) {  // Removes the least recently used node (tail) from the cache.
    if (!cache->tail) return;       // If the cache is empty, do nothing.
    Node* tail = cache->tail;       // Get the current tail node.
    if (tail->prev) tail->prev->next = NULL; // Update the second-last node to point to NULL.
    cache->tail = tail->prev;       // Update the tail pointer to the second-last node.
    if (cache->head == tail) cache->head = NULL; // If there was only one node, set head to NULL.
    free(tail);                     // Free the memory of the removed node.
    cache->size--;                  // Decrease the cache size by 1.
}

void addPage(LRUCache* cache, int page) { // Adds a new page to the head of the cache.
    Node* node = (Node*)malloc(sizeof(Node)); // Allocates memory for the new node.
    node->page = page;                // Stores the page number in the node.
    node->next = cache->head;         // Sets the new node's next pointer to the current head.
    node->prev = NULL;                // Sets the new node's previous pointer to NULL.
    if (cache->head) cache->head->prev = node; // Updates the old head’s previous pointer to the new node.
    cache->head = node;               // Sets the new node as the head of the cache.
    if (!cache->tail) cache->tail = node; // If the cache was empty, set the new node as the tail too.
    cache->size++;                    // Increment the cache size by 1.
}

int accessPage(LRUCache* cache, int page) { // Accesses a page, updating its position or adding it.
    Node* current = cache->head;      // Starts at the head of the cache.
    while (current) {                 // Iterates through the cache nodes.
        if (current->page == page) {  // If the page is found in the cache:
            moveToHead(cache, current); // Move it to the head (most recently used).
            return 0;                 // Return 0 (no page fault occurred).
        }
        current = current->next;      // Move to the next node in the cache.
    }
    if (cache->size == cache->capacity) { // If the cache is full:
        removeTail(cache);            // Remove the least recently used page (tail).
    }
    addPage(cache, page);             // Add the new page to the cache.
    return 1;                         // Return 1 (page fault occurred).
}

int main(int argc, char* argv[]) { // Main function to run the LRU simulation.
    if (argc != 4) {               // Checks if the correct number of arguments is provided.
        fprintf(stderr, "Usage: %s <num_pages> <page_size> <trace_file>\n", argv[0]); // Prints usage.
        return 1;                  // Exits with error if arguments are incorrect.
    }

    int num_pages = atoi(argv[1]); // Parses the number of physical pages.
    int page_size = atoi(argv[2]); // Parses the page size.
    char* filename = argv[3];      // Gets the filename of the trace file.

    printf("No physical pages = %d, page size = %d\n", num_pages, page_size); // Prints input details.
    printf("Reading memory trace from %s...", filename); // Indicates trace file being read.

    FILE* file = fopen(filename, "r"); // Opens the trace file for reading.
    if (!file) {                       // If the file cannot be opened:
        perror("Error opening file");  // Prints the error.
        return 1;                      // Exits with error.
    }

    LRUCache* cache = createCache(num_pages); // Creates an LRU cache with the specified number of pages.
    int pageFaults = 0;               // Initializes page fault counter to 0.
    unsigned int address;             // Variable to store each memory reference address.

    while (fscanf(file, "%u", &address) == 1) { // Reads memory addresses from the file.
        int page = address / page_size; // Converts memory address to a page number.
        pageFaults += accessPage(cache, page); // Accesses the page and increments faults if necessary.
    }

    fclose(file);                     // Closes the trace file after reading.
    printf(" Read %d memory references\n", pageFaults); // Prints total memory references.
    printf("Result: %d page faults\n", pageFaults); // Prints the total number of page faults.

    return 0;                         // Returns 0 to indicate successful execution.
}
