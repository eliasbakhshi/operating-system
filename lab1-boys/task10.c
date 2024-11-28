#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Definiera antal filosofer
#define NUM_PHILOSOPHERS 5

// Deklarera en array av mutexar för gafflarna
pthread_mutex_t forks[NUM_PHILOSOPHERS];

// Funktion som ska köras av varje filosoftråd
void* philosopher_task(void* arg) {
    int philosopher_id = *(int*)arg; // Hämta filosofens ID
    int left_fork = philosopher_id; // Den vänstra gaffeln är densamma som filosofens ID
    int right_fork = (philosopher_id + 1) % NUM_PHILOSOPHERS; // Den högra gaffeln är nästa i cirkeln

    while (1) {
        // Filosofen tänker
        printf("Philosopher %d: thinking\n", philosopher_id);
        sleep(rand() % 5 + 1);  // Tänk en slumpmässig tid mellan 1-5 sekunder

        // Försök ta vänster gaffel
        printf("Philosopher %d: attempting to pick up left fork\n", philosopher_id);
        pthread_mutex_lock(&forks[left_fork]); // Lås vänster gaffel
        printf("Philosopher %d: picked up left fork\n", philosopher_id);

        // Tänk en stund till
        sleep(rand() % 7 + 2);  // Tänk en slumpmässig tid mellan 2-8 sekunder

        // Försök ta höger gaffel
        printf("Philosopher %d: attempting to pick up right fork\n", philosopher_id);
        pthread_mutex_lock(&forks[right_fork]); // Lås höger gaffel
        printf("Philosopher %d: picked up right fork - eating\n", philosopher_id);

        // Äta
        sleep(rand() % 6 + 5);  // Ät en slumpmässig tid mellan 5-10 sekunder

        // Släpp gafflarna
        pthread_mutex_unlock(&forks[left_fork]); // Lås upp vänster gaffel
        pthread_mutex_unlock(&forks[right_fork]); // Lås upp höger gaffel
        printf("Philosopher %d: finished eating - back to thinking\n", philosopher_id);
    }

    return NULL;
}

// Huvudfunktion
int main() {
    srand(time(NULL)); // Initiera slumptalsgenerator
    pthread_t philosophers[NUM_PHILOSOPHERS]; // Array för filosoftrådar
    int philosopher_ids[NUM_PHILOSOPHERS]; // Array för filosofernas ID

    // Initiera mutexar för gafflarna
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&forks[i], NULL); // Initiera mutex för varje gaffel
    }

    // Skapa filosoftrådar
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosopher_ids[i] = i; // Tilldela filosofens ID
        pthread_create(&philosophers[i], NULL, philosopher_task, &philosopher_ids[i]); // Skapa filosoftråd
    }

    // Vänta på att trådarna ska avslutas (de kör dock oändligt)
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL); // Vänta på att filosoftråden avslutas
    }

    return 0; // Returnera 0 för att indikera att programmet avslutats korrekt
}
