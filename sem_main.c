#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOG "LOG: "
#define WARNING "WARN: "
#define ERROR "ERROR: "

const static int THREAD_COUNT = 10;
const static int THREAD_ITER_COUNT = 5;

int RESOURCE_COUNT = 3;
int THREAD_RES_TAKE = 1;

sem_t sem;

int rand_in_range(int min, int max) { return (rand() % (max - min + 1)) + min; }

// Tries to take count resources from the RESOURCE_COUNT
// Returns 0 on success -1 on failure.
int decrease_count(int count) {
  if (RESOURCE_COUNT < count) {
    return -1;
  } else {
    RESOURCE_COUNT -= count;
    return 0;
  }
}

// Return the specified count to the resources
void increase_count(int count) { RESOURCE_COUNT += count; }

void *thread_main(void *p) {
  int id = *(int *)p;
  fprintf(stderr, LOG "Thread %d: STARTING!\n", id);

  for (int i = 0; i < THREAD_ITER_COUNT; i++) {
    // Take resource...
    fprintf(stderr, LOG "Thread %d: Taking resource! Iteration %d/%d\n", id,
            i + 1, THREAD_ITER_COUNT);

    int decrease_result = -1;
    while (0 != decrease_result) {
      fprintf(stderr, "Thread %d: Attempting to get resource...\n", id);

      if (0 != sem_wait(&sem)) {
        fprintf(stderr, ERROR "Thread %d: Error waiting for semaphore!\n", id);
        exit(1);
      }

      decrease_result = decrease_count(THREAD_RES_TAKE);

      if (0 != sem_post(&sem)) {
        fprintf(stderr, ERROR "Thread %d: Error releasing for semaphore!\n",
                id);
        exit(1);
      }

      if (0 != decrease_result) {
        struct timespec time = {.tv_nsec = 0, .tv_sec = 1};
        fprintf(stderr,
                WARNING "Thread %d: No resources available, retrying after a "
                        "quick sleep of %lds!\n",
                id, time.tv_sec);
        nanosleep(&time, NULL);
      }
    }
    fprintf(stderr, "Thread %d: Got resource\n", id);

    // Do some work...
    struct timespec time = {.tv_nsec = rand_in_range(0, 980),
                            .tv_sec = rand_in_range(1, 5)};
    fprintf(stderr, LOG "Thread %d: Working for %ld nanosecs and %ld secs\n",
            id, time.tv_nsec, time.tv_sec);
    if (0 != nanosleep(&time, NULL)) {
      fprintf(stderr,
              ERROR "Thread %d: Had an error doing work (error waiting)...\n",
              id);
      exit(1);
    }

    // Return resource...
    fprintf(stderr, LOG "Thread %d: Returning resource!\n", id);
    if (0 != sem_wait(&sem)) {
      fprintf(stderr, ERROR "Thread %d: Error waiting for semaphore!\n", id);
      exit(1);
    }
    increase_count(THREAD_RES_TAKE);
    if (0 != sem_post(&sem)) {
      fprintf(stderr, ERROR "Thread %d: Error releasing for semaphore!\n", id);
      exit(1);
    }
  }

  fprintf(stderr, LOG "Thread %d: DONE!\n", id);
  return NULL;
}

int main() {
  fprintf(stderr, LOG "Initializing semaphore...\n");
  if (0 != sem_init(&sem, 0, 1)) {
    fprintf(stderr, ERROR "An error ocurred initializing semaphore...\n");
    return 1;
  }

  fprintf(stderr, LOG "Creating pthreads...\n");
  pthread_t thread_ids[THREAD_COUNT];
  int inner_ids[THREAD_COUNT];
  for (int i = 0; i < THREAD_COUNT; i++) {
    inner_ids[i] = i + 1;
    if (pthread_create(thread_ids + i, NULL, thread_main, inner_ids + i) != 0) {
      fprintf(stderr, ERROR "An error ocurred creating thread # %d!\n", i);
      return 1;
    }
  }

  fprintf(stderr, LOG "Waiting for pthreads...\n");
  for (int i = 0; i < THREAD_COUNT; i++) {
    pthread_join(thread_ids[i], NULL);
  }

  fprintf(stderr, LOG "Destroying semaphore...\n");
  if (0 != sem_destroy(&sem)) {
    fprintf(stderr, ERROR "Failed to destroy semaphore!\n");
    return 1;
  }

  fprintf(stderr, LOG "DONE!\n");
}
