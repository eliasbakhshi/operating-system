// Include necessary libraries for input/output, dynamic memory allocation, and threads
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Define the size of the matrices
#define SIZE 1024

// Declare static matrices a, b, and c
static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

// Thread function prototype
void *multiply_row(void *arg);

// Function to initialize matrices a and b
static void init_matrix(void)
{
    int i, j;

    // Loop over each element in the matrices
    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++) {
            a[i][j] = 1.0; // Set each element in matrix a to 1.0
            b[i][j] = 1.0; // Set each element in matrix b to 1.0
        }
}
// Function to print the matrix
static void print_matrix(void) {
    int i, j; // Declare variables for the loop counters

    // Loop over each row of the matrix
    for (i = 0; i < SIZE; i++) {
        // Loop over each column of the matrix
        for (j = 0; j < SIZE; j++)
            printf(" %7.2f", c[i][j]); // Print the matrix element at row i, column j with a width of 7 characters and 2 decimal places
        printf("\n"); // Print a newline character at the end of each row
    }
}

// Function to be executed by each thread
void *multiply_row(void *arg)
{
    int i = *(int *)arg; // Get the row number
    int j, k;

    // Loop over each element in the row
    for (j = 0; j < SIZE; j++) {
        c[i][j] = 0.0; // Initialize the element in matrix c to 0.0
        for (k = 0; k < SIZE; k++)
            c[i][j] += a[i][k] * b[k][j]; // Multiply the corresponding elements in matrices a and b and add the result to the element in matrix c
    }

    pthread_exit(NULL); // Exit the thread
}

// Main function
int main(int argc, char **argv)
{
    pthread_t threads[SIZE]; // Array of threads
    int thread_args[SIZE]; // Array of thread arguments
    int i, rc;

    init_matrix(); // Initialize matrices a and b

    // Create one thread for each row in the matrices
    for (i = 0; i < SIZE; i++) {
        thread_args[i] = i; // Set the thread argument to the row number
        rc = pthread_create(&threads[i], NULL, multiply_row, (void *)&thread_args[i]); // Create the thread
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc); // Print an error message if the thread could not be created
            exit(-1); // Exit the program
        }
    }

    // Wait for all threads to complete
    for (i = 0; i < SIZE; i++) {
        pthread_join(threads[i], NULL); // Wait for the thread to finish
    }

    // Optionally print the result matrix
    // print_matrix();

    return 0; // Return 0 to indicate successful execution
}