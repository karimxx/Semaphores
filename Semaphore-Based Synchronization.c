#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 5         // Number of mCounter threads
#define BUFFER_SIZE 10  // Size of the buffer

int counter = 0;
int buffer[BUFFER_SIZE];
int buffer_index = 0;

sem_t counter_mutex;  // Mutex for counter access
sem_t buffer_mutex;   // Mutex for buffer access
sem_t buffer_slots;   // Semaphore to track available slots in the buffer
sem_t buffer_items;   // Semaphore to track available items in the buffer

void *mCounter_thread(void *arg) {
    int id = *(int *)arg;
    while (1) {
        sleep(rand() % 5);  // Random sleep to simulate message arrival
        printf("Counter thread %d: received a message\n", id);

        printf("Counter thread %d: waiting to write\n", id);
        sem_wait(&counter_mutex);  // Wait for access to counter

        counter++;
        printf("Counter thread %d: now adding to counter, counter value=%d\n", id, counter);

        sem_post(&counter_mutex);  // Release access to counter
    }
}

void *mMonitor_thread(void *arg) {
    while (1) {
        sleep(rand() % 5);  // Random sleep for monitor activation

        printf("Monitor thread: waiting to read counter\n");
        sem_wait(&counter_mutex);  // Wait for access to counter

        int count = counter;
        counter = 0;
        printf("Monitor thread: reading a count value of %d\n", count);

        sem_post(&counter_mutex);  // Release access to counter

        sem_wait(&buffer_slots);  // Wait for an available slot in the buffer
        sem_wait(&buffer_mutex);  // Wait for access to buffer

        if (buffer_index < BUFFER_SIZE) {
            buffer[buffer_index++] = count;
            printf("Monitor thread: writing to buffer at position %d\n", buffer_index - 1);
        } else {
            printf("Monitor thread: Buffer full!!\n");
        }

        sem_post(&buffer_mutex);  // Release access to buffer
        sem_post(&buffer_items);  // Signal that an item is available in the buffer
    }
}

void *mCollector_thread(void *arg) {
    while (1) {
        sleep(rand() % 5);  // Random sleep for collector activation

        sem_wait(&buffer_items);  // Wait for an available item in the buffer
        sem_wait(&buffer_mutex);  // Wait for access to buffer

        if (buffer_index > 0) {
            int value = buffer[--buffer_index];
            printf("Collector thread: reading from the buffer at position %d\n", buffer_index);
        } else {
            printf("Collector thread: nothing is in the buffer!\n");
        }

        sem_post(&buffer_mutex);  // Release access to buffer
        sem_post(&buffer_slots);  // Signal that a slot is available in the buffer
    }
}

int main() {
    pthread_t mCounter_threads[N];
    pthread_t mMonitor_thread_id;
    pthread_t mCollector_thread_id;

    sem_init(&counter_mutex, 0, 1);  
    sem_init(&buffer_mutex, 0, 1);  
    sem_init(&buffer_slots, 0, BUFFER_SIZE);  
    sem_init(&buffer_items, 0, 0);   

    for (int i = 0; i < N; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&mCounter_threads[i], NULL, mCounter_thread, id);
    }

    pthread_create(&mMonitor_thread_id, NULL, mMonitor_thread, NULL);
    pthread_create(&mCollector_thread_id, NULL, mCollector_thread, NULL);

    for (int i = 0; i < N; i++) {
        pthread_join(mCounter_threads[i], NULL);
    }
    pthread_join(mMonitor_thread_id, NULL);
    pthread_join(mCollector_thread_id, NULL);

    sem_destroy(&counter_mutex);
    sem_destroy(&buffer_mutex);
    sem_destroy(&buffer_slots);
    sem_destroy(&buffer_items);

    return 0;
}
