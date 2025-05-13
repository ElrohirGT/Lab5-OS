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

int THREAD_RES_TAKE = 5;
int RESOURCE_COUNT = 50;

int rand_in_range(int min, int max) { return (rand() % (max - min + 1)) + min; }

typedef struct {
  sem_t sem;
  int resource_count;
} Monitor;

Monitor GLOBAL_MONITOR;

int monitor_init(Monitor *mon) {
  if (0 != sem_init(&mon->sem, 0, 1)) {
    fprintf(stderr, ERROR "An error ocurred initializing semaphore...\n");
    return 1;
  }
  mon->resource_count = RESOURCE_COUNT;

  return 0;
}

int monitor_deinit(Monitor *mon) {
  if (0 != sem_destroy(&mon->sem)) {
    fprintf(stderr, ERROR "Failed to destroy semaphore!\n");
    return 1;
  }

  return 0;
}

// Tries to take count resources from the RESOURCE_COUNT
// Returns 0 on success -1 on failure.
int decrease_count(Monitor *mon, int count) {
  fprintf(stderr, LOG "MONITOR: Trying to decrease count...");
  int failed = 0;

  if (0 != sem_wait(&mon->sem)) {
    fprintf(stderr, ERROR "MONITOR: Failed to wait on the semaphore!");
    return -1;
  }

  if (mon->resource_count < count) {
    fprintf(stderr, WARNING "MONITOR: No resources available!");
    failed = -1;
  } else {
    mon->resource_count -= count;
    fprintf(stderr, LOG "MONITOR: COUNT DECREASED, %d LEFT...",
            mon->resource_count);
  }

  if (0 != sem_post(&mon->sem)) {
    fprintf(stderr, ERROR "MONITOR: Failed to release on the semaphore!");
    failed = -1;
  }

  return failed;
}

int get_count(Monitor *mon) {
  if (0 != sem_wait(&mon->sem)) {
    return -1;
  }

  int count = mon->resource_count;

  if (0 != sem_post(&mon->sem)) {
    return -1;
  }

  return count;
}

// Return the specified count to the resources
int increase_count(Monitor *mon, int count) {
  fprintf(stderr, LOG "Entered monitor increase_count\n");
  int failed = 0;

  if (0 != sem_wait(&mon->sem)) {
    return -1;
  }

  mon->resource_count += count;

  if (0 != sem_post(&mon->sem)) {
    failed = -1;
  }

  return failed;
}

void *thread_main(void *p) {
  int id = *(int *)p;
  fprintf(stderr, LOG "Thread %d: STARTING!\n", id);

  for (int i = 0; i < THREAD_ITER_COUNT; i++) {
    // Take resource...
    fprintf(stderr, LOG "Thread %d: Taking %d resources! Iteration %d/%d\n", id,
            THREAD_RES_TAKE, i + 1, THREAD_ITER_COUNT);
    int decrease_result = -1;
    while (0 != decrease_result) {
      decrease_result = decrease_count(&GLOBAL_MONITOR, THREAD_RES_TAKE);
      if (0 != decrease_result) {
        struct timespec time = {.tv_sec = 1, .tv_nsec = 0};
        fprintf(stderr,
                WARNING "Thread %d: Failed to take resource from "
                        "monitor, retrying after a quick sleep of %lds!\n",
                id, time.tv_sec);
        nanosleep(&time, NULL);
      }
    }

    if (0 != decrease_count(&GLOBAL_MONITOR, THREAD_RES_TAKE)) {
      fprintf(stderr,
              WARNING
              "Thread %d: Failed to take resources from monitor, retrying!\n",
              id);
      continue;
    }
    fprintf(stderr, LOG "Thread %d: Resources remaining: %d\n", id,
            get_count(&GLOBAL_MONITOR));

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
    increase_count(&GLOBAL_MONITOR, THREAD_RES_TAKE);
  }

  fprintf(stderr, LOG "Thread %d: DONE!\n", id);
  return NULL;
}

int main() {
  fprintf(stderr, LOG "Initializing Monitor...\n");
  if (0 != monitor_init(&GLOBAL_MONITOR)) {
    fprintf(stderr, ERROR "An error ocurred initializing monitor...\n");
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

  fprintf(stderr, LOG "Destroying monitor...\n");
  if (0 != monitor_deinit(&GLOBAL_MONITOR)) {
    fprintf(stderr, ERROR "An error ocurred destroying monitor!\n");
    return 1;
  }

  fprintf(stderr, LOG "DONE!\n");
}
