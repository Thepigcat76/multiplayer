#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Function executed by the thread
void* my_thread_function(void* arg) {
    int value = *(int*)arg;
    printf("Thread received value: %d\n", value);
    return NULL;
}

void threads_example() {
    pthread_t thread;
    int arg = 42;

    // Create the thread
    if (pthread_create(&thread, NULL, my_thread_function, &arg) != 0) {
        perror("Failed to create thread");
        exit(1);
    }

    // Wait for the thread to finish
    pthread_join(thread, NULL);

    printf("Thread has finished\n");
}
