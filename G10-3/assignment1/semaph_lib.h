#ifndef MYSEM_H
#define MYSEM_H

#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>

// Define semaphore structure
typedef struct {
    int semid;
    int initialized;
    volatile int val;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} mysem_t;

// Function prototypes
int mysem_init(mysem_t *s, int n);
int mysem_down(mysem_t *s);
int mysem_up(mysem_t *s);
int mysem_destroy(mysem_t *s);

#endif // MYSEM_H
