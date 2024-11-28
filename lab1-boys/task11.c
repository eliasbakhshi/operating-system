#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Definiera antal filosofer
#define NUM_PHILOSOPHERS 5

// Deklarera en array av mutexar för gafflarna
pthread_mutex_t forks[NUM_PHILOSOPHERS];

// Funktion som varje filosoftråd ska köra
void* philosopher_activity(void* arg) {
    int philosopher_id = *(int*)arg; // Hämta filosofens ID
    int first_fork, second_fork;

    // Särskilt fall för sista filosofen för att undvika dödläge
    if (philosopher_id == NUM_PHILOSOPHERS - 1) {
        first_fork = 0; // Den första gaffeln är den första i arrayen
        second_fork = philosopher_id; // Den andra gaffeln är samma som filosofens ID
    } else {
        first_fork = philosopher_id; // Den första gaffeln är samma som filosofens ID
        second_fork = (philosopher_id + 1) % NUM_PHILOSOPHERS; // Den andra gaffeln är nästa i arrayen
    }

    while (1) { // Oändlig loop
        // Filosofen tänker
        printf("Philosopher %d: contemplating\n", philosopher_id);
        sleep(rand() % 5 + 1);  // Tänker i en slumpmässig tid mellan 1-5 sekunder

        // Försök ta den första gaffeln
        printf("Philosopher %d: trying to acquire fork %d\n", philosopher_id, first_fork);
        pthread_mutex_lock(&forks[first_fork]); // Lås den första gaffelns mutex
        printf("Philosopher %d: acquired fork %d\n", philosopher_id, first_fork);

        // Tänk lite till
        sleep(rand() % 7 + 2);  // Tänker i en slumpmässig tid mellan 2-8 sekunder

        // Försök ta den andra gaffeln
        printf("Philosopher %d: trying to acquire fork %d\n", philosopher_id, second_fork);
        pthread_mutex_lock(&forks[second_fork]); // Lås den andra gaffelns mutex
        printf("Philosopher %d: acquired fork %d - dining\n", philosopher_id, second_fork);

        // Äta
        sleep(rand() % 6 + 5);  // Äter i en slumpmässig tid mellan 5-10 sekunder

        // Lämna tillbaka gafflarna
        pthread_mutex_unlock(&forks[first_fork]); // Lås upp den första gaffeln
        pthread_mutex_unlock(&forks[second_fork]); // Lås upp den andra gaffeln
        printf("Philosopher %d: finished dining - back to contemplation\n", philosopher_id);
    }

    return NULL; // Returnera NULL från trådfunktionen
}

// Huvudfunktion
int main() {
    srand(time(NULL)); // Initiera slumptalsgenerator för att skapa slumpmässiga väntetider

    pthread_t philosophers[NUM_PHILOSOPHERS]; // Array för filosoftrådar

    int philosopher_ids[NUM_PHILOSOPHERS]; // Array för att hålla ID:n för filosoferna

    // Initiera mutexar för gafflarna
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&forks[i], NULL); // Initiera varje gaffel-mutex i arrayen
    }

    // Skapa filosoftrådar
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosopher_ids[i] = i; // Tilldela ett ID till varje filosof
        pthread_create(&philosophers[i], NULL, philosopher_activity, &philosopher_ids[i]); // Skapa en tråd för varje filosof och skicka in filosofens rutin och ID
    }

    // Vänta på trådarna (de kör dock oändligt)
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL); // Vänta på att varje filosoftråd ska avslutas (men de kör oändligt)
    }

    return 0; // Returnera 0 för att indikera lyckad körning
}
