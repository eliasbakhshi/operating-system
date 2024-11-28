#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Definiera storleken på matriserna
#define MATRIX_SIZE 1024

// Deklarera statiska matriser x, y och result
static double x[MATRIX_SIZE][MATRIX_SIZE];
static double y[MATRIX_SIZE][MATRIX_SIZE];
static double result[MATRIX_SIZE][MATRIX_SIZE];

// Trådfunktionsprototyp
void *compute_row(void *arg);

// Funktion för att initiera matriserna x och y
static void initialize_matrices(void)
{
    int row, col;

    // Loop över varje element i matriserna
    for (row = 0; row < MATRIX_SIZE; row++)
        for (col = 0; col < MATRIX_SIZE; col++) {
            x[row][col] = 1.0; // Sätt varje element i matris x till 1.0
            y[row][col] = 1.0; // Sätt varje element i matris y till 1.0
        }
}

// Funktion för att skriva ut matrisen result
static void display_matrix(void) {
    int row, col; // Deklarera variabler för looparna

    // Loop över varje rad i matrisen
    for (row = 0; row < MATRIX_SIZE; row++) {
        // Loop över varje kolumn i matrisen
        for (col = 0; col < MATRIX_SIZE; col++)
            printf(" %7.2f", result[row][col]); // Skriv ut elementet i matrisen med rad row, kolumn col, med 7 tecken bredd och 2 decimaler
        printf("\n"); // Ny rad efter varje rad i matrisen
    }
}

// Funktion som körs av varje tråd
void *compute_row(void *arg)
{
    int row_idx = *(int *)arg; // Hämta radnumret
    int col, inner;

    // Loop över varje element i raden
    for (col = 0; col < MATRIX_SIZE; col++) {
        result[row_idx][col] = 0.0; // Initiera elementet i matrisen result till 0.0
        for (inner = 0; inner < MATRIX_SIZE; inner++)
            result[row_idx][col] += x[row_idx][inner] * y[inner][col]; // Multiplicera motsvarande element i matriserna x och y och addera till elementet i matris result
    }

    pthread_exit(NULL); // Avsluta tråden
}

// Huvudfunktion
int main(int argc, char **argv)
{
    pthread_t threads[MATRIX_SIZE]; // Array av trådar
    int row_indices[MATRIX_SIZE]; // Array för trådargument
    int i, status_code;

    initialize_matrices(); // Initiera matriserna x och y
    // display_matrix();

    // Skapa en tråd för varje rad i matriserna
    for (i = 0; i < MATRIX_SIZE; i++) {
        row_indices[i] = i; // Tilldela trådargumentet till radnumret
        status_code = pthread_create(&threads[i], NULL, compute_row, (void *)&row_indices[i]); // Skapa tråden
        if (status_code) {
            printf("FEL; returvärde från pthread_create() är %d\n", status_code); // Skriv ut ett felmeddelande om tråden inte kunde skapas
            exit(-1); // Avsluta programmet
        }
    }

    // Vänta på att alla trådar avslutas
    for (i = 0; i < MATRIX_SIZE; i++) {
        pthread_join(threads[i], NULL); // Vänta på att tråden avslutas
    }

    // Valfritt: skriv ut resultatmatrisen
    // display_matrix();

    return 0; // Returnera 0 för att indikera att programmet avslutades framgångsrikt
}
