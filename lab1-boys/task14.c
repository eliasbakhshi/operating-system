#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Definiera storleken på matriserna
#define MATRIX_SIZE 1024

// Deklarera statiska matriser x, y och z
static double x[MATRIX_SIZE][MATRIX_SIZE];
static double y[MATRIX_SIZE][MATRIX_SIZE];
static double z[MATRIX_SIZE][MATRIX_SIZE];

// Funktionprototyper för initiering och multiplikation
void *initialize_matrix(void *arg);
void *compute_row(void *arg);

// Funktion som varje initieringstråd ska köra
void *initialize_matrix(void *arg)
{
    int row_idx = *(int *)arg; // Hämta radnumret
    int col;

    // Loop över varje element i raden
    for (col = 0; col < MATRIX_SIZE; col++) {
        x[row_idx][col] = 1.0; // Sätt elementet i matris x till 1.0
        y[row_idx][col] = 1.0; // Sätt elementet i matris y till 1.0
    }

    pthread_exit(NULL); // Avsluta tråden
}

// Funktion som varje multiplikationstråd ska köra
void *compute_row(void *arg)
{
    int row_idx = *(int *)arg; // Hämta radnumret
    int col, inner;

    // Loop över varje element i raden
    for (col = 0; col < MATRIX_SIZE; col++) {
        z[row_idx][col] = 0.0; // Initiera elementet i matris z till 0.0
        for (inner = 0; inner < MATRIX_SIZE; inner++)
            z[row_idx][col] += x[row_idx][inner] * y[inner][col]; // Multiplicera motsvarande element i matriserna x och y och lägg till resultatet i matris z
    }

    pthread_exit(NULL); // Avsluta tråden
}

// Huvudfunktion
int main(int argc, char **argv)
{
    pthread_t init_threads[MATRIX_SIZE], mult_threads[MATRIX_SIZE]; // Arrays för initierings- och multiplikationstrådar
    int row_indices[MATRIX_SIZE]; // Array av trådargument
    int i, status_code;

    // Parallell initiering av matriser
    for (i = 0; i < MATRIX_SIZE; i++) {
        row_indices[i] = i; // Tilldela trådargumentet till radnumret
        status_code = pthread_create(&init_threads[i], NULL, initialize_matrix, (void *)&row_indices[i]); // Skapa initieringstråd
        if (status_code) {
            printf("FEL; returvärde från pthread_create() är %d\n", status_code); // Skriv ut ett felmeddelande om tråden inte kunde skapas
            exit(-1); // Avsluta programmet
        }
    }

    // Vänta på att alla initieringstrådar ska avslutas
    for (i = 0; i < MATRIX_SIZE; i++) {
        pthread_join(init_threads[i], NULL); // Vänta på att initieringstråden ska avslutas
    }

    // Parallell matris-multiplikation
    for (i = 0; i < MATRIX_SIZE; i++) {
        status_code = pthread_create(&mult_threads[i], NULL, compute_row, (void *)&row_indices[i]); // Skapa multiplikationstråd
        if (status_code) {
            printf("FEL; returvärde från pthread_create() är %d\n", status_code); // Skriv ut ett felmeddelande om tråden inte kunde skapas
            exit(-1); // Avsluta programmet
        }
    }

    // Vänta på att alla multiplikationstrådar ska avslutas
    for (i = 0; i < MATRIX_SIZE; i++) {
        pthread_join(mult_threads[i], NULL); // Vänta på att multiplikationstråden ska avslutas
    }

    // Valfritt: skriv ut resultatmatrisen
    // display_matrix();

    return 0; // Returnera 0 för att indikera att programmet avslutades framgångsrikt
}
