#include "semaph_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;

static char path[] = "/tmp/mysem";  // Update this path if necessary
static int sem_num = 0;

int mysem_init(mysem_t *s, int n) {
    key_t key;

    pthread_mutex_lock(&mtx);

    if (s->initialized) {
        pthread_mutex_unlock(&mtx);
        return -1;
    }

    if (n != 0 && n != 1) {
        pthread_mutex_unlock(&mtx);
        return 0;
    }

    sem_num++;
    key = ftok(path, sem_num);
    s->semid = semget(key, 1, S_IRWXU | IPC_CREAT);
    if (s->semid == -1) {
        perror("semget failed");
        pthread_mutex_unlock(&mtx);
        return -1;
    }

    if (semctl(s->semid, 0, SETVAL, n) == -1) {
        perror("semctl SETVAL failed");
        pthread_mutex_unlock(&mtx);
        return -1;
    }

    s->initialized = 1;
    pthread_mutex_unlock(&mtx);

    return 1;
}

int mysem_down(mysem_t *s) {
    struct sembuf op;

    if (!s->initialized)
        return -1;

    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;

    if (semop(s->semid, &op, 1) == -1) {
        perror("semop down failed");
        return -1;
    }

    return 1;
}

int mysem_up(mysem_t *s) {
    struct sembuf op;

    if (!s->initialized)
        return -1;

    pthread_mutex_lock(&mtx2);

    if (semctl(s->semid, 0, GETVAL) == -1) {
        perror("semctl GETVAL failed");
        pthread_mutex_unlock(&mtx2);
        return -1;
    }

    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;

    if (semop(s->semid, &op, 1) == -1) {
        perror("semop up failed");
        pthread_mutex_unlock(&mtx2);
        return -1;
    }

    pthread_mutex_unlock(&mtx2);

    return 1;
}

int mysem_destroy(mysem_t *s) {
    if (!s->initialized)
        return -1;

    pthread_mutex_lock(&mtx);

    if (semctl(s->semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed");
        pthread_mutex_unlock(&mtx);
        return -1;
    }

    s->initialized = 0;
    s->semid = 0;

    pthread_mutex_unlock(&mtx);

    return 1;
}
