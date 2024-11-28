#include <stdio.h>                  // Includes standard input/output functions.
#include <stdlib.h>                 // Includes memory allocation and utility functions.
#include <stdbool.h>                // Enables usage of `true` and `false` in the code.

// Function to find the page to replace based on future usage
int findPageToReplace(int *frames, int frameCount, int *references, int currentIndex, int totalRefs) {
    int farthest = -1;              // Tracks the farthest future reference of any page in frames.
    int replaceIndex = -1;          // Index of the page to be replaced.

    for (int i = 0; i < frameCount; i++) { // Iterate over all frames.
        int j;
        for (j = currentIndex + 1; j < totalRefs; j++) { // Search for future references of the current page.
            if (frames[i] == references[j]) { // If the page in frame is found in future references:
                if (j > farthest) {           // Check if it's the farthest found so far.
                    farthest = j;             // Update the farthest reference.
                    replaceIndex = i;         // Update the index of the page to replace.
                }
                break;                        // Stop checking further references for this page.
            }
        }
        // If the page is never used in the future, return it immediately
        if (j == totalRefs) {
            return i;                         // Return the index of the unused page.
        }
    }

    return (replaceIndex == -1) ? 0 : replaceIndex; // If no page was marked, replace the first one (default).
}

// Function to check if a page is already in memory
bool isPageInMemory(int *frames, int frameCount, int page) {
    for (int i = 0; i < frameCount; i++) { // Iterate through the frames.
        if (frames[i] == page) {           // Check if the current frame contains the page.
            return true;                   // Return true if page is found in memory.
        }
    }
    return false;                          // Return false if the page is not in memory.
}

// Function to simulate the Optimal Page Replacement Algorithm
int optimalPageReplacement(int *references, int totalRefs, int frameCount) {
    int *frames = (int *)malloc(frameCount * sizeof(int)); // Allocate memory for the frames array.
    for (int i = 0; i < frameCount; i++) { // Initialize all frames to -1 (empty).
        frames[i] = -1;
    }

    int pageFaults = 0;                    // Initialize the page fault counter.

    for (int i = 0; i < totalRefs; i++) {  // Iterate over all memory references.
        int currentPage = references[i];  // Get the current page from the references.

        if (!isPageInMemory(frames, frameCount, currentPage)) { // Check if the page is in memory.
            pageFaults++;               // Increment the page fault counter if not in memory.

            if (i >= frameCount) {      // If all frames are occupied:
                int replaceIndex = findPageToReplace(frames, frameCount, references, i, totalRefs); // Find the best page to replace.
                frames[replaceIndex] = currentPage; // Replace the selected page with the current page.
            } else {
                frames[i] = currentPage; // If frames are not full, add the page directly.
            }
        }
    }

    free(frames);                        // Free the memory allocated for the frames array.
    return pageFaults;                   // Return the total number of page faults.
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 4) {                     // Check if the correct number of arguments is provided.
        fprintf(stderr, "Usage: %s no_phys_pages page_size filename\n", argv[0]); // Print usage.
        return 1;                        // Exit with error if arguments are incorrect.
    }

    int no_phys_pages = atoi(argv[1]);   // Parse the number of physical pages from arguments.
    int page_size = atoi(argv[2]);       // Parse the page size from arguments.
    const char *filename = argv[3];      // Get the filename of the trace file.

    printf("No physical pages = %d, page size = %d\n", no_phys_pages, page_size); // Print input details.
    printf("Reading memory trace from %s...\n", filename); // Indicate the start of trace reading.

    // Open the memory trace file
    FILE *file = fopen(filename, "r");   // Open the trace file for reading.
    if (!file) {                         // Check if the file could not be opened.
        perror("Error opening file");    // Print the error message.
        return 1;                        // Exit with an error code.
    }

    // Read memory references from the file
    int *references = (int *)malloc(100000 * sizeof(int)); // Allocate memory for storing references (max 100000).
    int totalRefs = 0;                  // Initialize the total reference counter.
    unsigned int address;               // Variable to store the address read from the file.

    while (fscanf(file, "%u", &address) == 1) { // Read memory addresses from the file.
        references[totalRefs++] = address / page_size; // Convert address to page number and store it.
    }

    fclose(file);                        // Close the trace file after reading.
    printf("Read %d memory references\n", totalRefs); // Print the total number of memory references read.

    // Perform Optimal Page Replacement and count page faults
    int pageFaults = optimalPageReplacement(references, totalRefs, no_phys_pages); // Calculate page faults.

    printf("Result: %d page faults\n", pageFaults); // Print the total number of page faults.

    free(references);                    // Free the memory allocated for the references array.
    return 0;                            // Return 0 to indicate successful execution.
}
