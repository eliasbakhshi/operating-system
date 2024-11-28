#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Initiera en mutex och en delad variabel för kontosaldo
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
double accountBalance = 0;

// Funktion för att sätta in pengar på kontot
void addAmount(double amount) {
    pthread_mutex_lock(&mutex); // Lås mutex
    accountBalance += amount; // Lägg till beloppet på kontot
    pthread_mutex_unlock(&mutex); // Lås upp mutex
}

// Funktion för att ta ut pengar från kontot
void removeAmount(double amount) {
    pthread_mutex_lock(&mutex); // Lås mutex
    accountBalance -= amount; // Ta ut beloppet från kontot
    pthread_mutex_unlock(&mutex); // Lås upp mutex
}

// Kontrollera om ett tal är udda
unsigned isOdd(unsigned long num) {
    return num % 2; // Returnera 1 om talet är udda, annars 0
}

// Funktion för att utföra 1000 transaktioner
void process1000Transactions(unsigned long threadID) {
    for (int i = 0; i < 1000; i++) {
        if (isOdd(threadID)) // Om tråd-ID är udda
            addAmount(100.00); // Sätt in $100
        else // Om tråd-ID är jämnt
            removeAmount(100.00); // Ta ut $100
    }
}

// Funktion som ska köras av varje tråd
void* threadTask(void* arg) {
    unsigned long tID = (unsigned long)arg; // Hämta tråd-ID
    process1000Transactions(tID); // Utför 1000 transaktioner
    return NULL; // Returnera NULL
}

// Huvudfunktion
int main(int argc, char** argv) {
    pthread_t *threads; // Array för trådar
    unsigned long i = 0; // Tråd-ID
    unsigned long totalThreads = 0; // Antal trådar

    // Kontrollera om antal trådar har skickats som argument
    if (argc > 1)
        totalThreads = atoi(argv[1]); // Omvandla argumentet till ett heltal

    // Allokera minne för trådar
    threads = malloc(totalThreads * sizeof(pthread_t));

    // Skapa trådar
    for (i = 0; i < totalThreads; i++) {
        if (pthread_create(&threads[i], NULL, threadTask, (void*)i) != 0) {
            perror("Error creating thread"); // Skriv ut systemfelmeddelande
            exit(EXIT_FAILURE); // Avsluta programmet
        }
    }

    // Vänta på att trådarna ska avslutas
    for (i = 0; i < totalThreads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread"); // Skriv ut systemfelmeddelande
            exit(EXIT_FAILURE); // Avsluta programmet
        }
    }

    // Skriv ut det slutgiltiga kontosaldot
    printf("Final balance in account: $%.2f\n", accountBalance);

    // Frigör allokerat minne och förstör mutex
    free(threads);
    pthread_mutex_destroy(&mutex);

    return 0; // Returnera 0 för att indikera att programmet avslutats korrekt
}
