#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Struktur för att hantera argument till trådarna
struct ThreadData {
    unsigned int tid;         // Trådens ID
    unsigned int totalThreads; // Totala antalet trådar
    unsigned int squaredTid;   // Kvadrerade värdet av trådens ID
};

// Funktion som ska köras av varje tråd
void* worker(void* input) {
    struct ThreadData *data = (struct ThreadData*) input; // Omvandla input till strukturen ThreadData
    unsigned int currentTid = data->tid; // Hämta trådens ID
    unsigned int total = data->totalThreads; // Hämta totalt antal trådar
    data->squaredTid = currentTid * currentTid; // Beräkna och lagra det kvadrerade ID-värdet
    printf("Greetings from worker #%u of %u\n", currentTid, total); // Hälsning från tråden
    return NULL; // Returnera NULL
}

// Huvudfunktion
int main(int argc, char** argv) {
    pthread_t* threads;            // Array för trådarna
    struct ThreadData* threadData;  // Array för trådens data
    unsigned int threadCount = 0;   // Antal trådar

    // Kontrollera om antal trådar har skickats som kommandoradsargument
    if (argc > 1)
        threadCount = atoi(argv[1]); // Omvandla argumentet till ett heltal

    // Allokera minne för trådar och deras data
    threads = malloc(threadCount * sizeof(pthread_t));
    threadData = malloc(threadCount * sizeof(struct ThreadData));

    // Skapa trådarna
    for (unsigned int i = 0; i < threadCount; i++) {
        threadData[i].tid = i; // Tilldela trådens ID
        threadData[i].totalThreads = threadCount; // Tilldela totalt antal trådar
        pthread_create(&(threads[i]), NULL, worker, (void*)&threadData[i]); // Skapa tråd
    }

    printf("I am the parent (main) thread.\n"); // Hälsning från huvudtråden

    // Vänta på trådarna och skriv ut deras kvadrerade ID
    for (unsigned int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL); // Vänta på att tråden ska avslutas
        printf("Worker #%u squared ID: %u\n", i, threadData[i].squaredTid); // Skriv ut trådens kvadrerade ID
    }
    

    // Frigör allokerat minne
    free(threadData);
    free(threads);

    return 0; // Returnera 0 för att indikera att programmet avslutats korrekt
}
