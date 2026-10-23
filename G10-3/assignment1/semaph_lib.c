#include "semaph_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static volatile int id = 0;

int mysem_init(mysem_t *s, int n) {

    pthread_mutex_lock(&s->mutex);

    if (s->initialized) {
        pthread_mutex_unlock(&s->mutex);
        return -1;
    }
    else if (n != 0 && n != 1) {
        pthread_mutex_unlock(&s->mutex);
        return 0;
    }

    id++;

    s->semid = id;
    s->val = n;
    s->initialized = 1;
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->cond, NULL);
    
    pthread_mutex_unlock(&s->mutex);

    return 1;
}

int mysem_down(mysem_t *s) {
    if (!s->initialized)
        return -1;

    pthread_mutex_lock(&s->mutex);

    while(s->val == 0) {
        pthread_cond_wait(&s->cond, &s->mutex);    
    }   
    s->val--;

    pthread_mutex_unlock(&s->mutex);

    return 1;
}

int mysem_up(mysem_t *s) {
    if (!s->initialized)
        return -1;  

    pthread_mutex_lock(&s->mutex);

    if(s->val)
        return 0;
    s->val++;
    pthread_cond_signal(&s->cond);

    pthread_mutex_unlock(&s->mutex);

    return 1;
}

int mysem_destroy(mysem_t *s) {
    if (!s->initialized)
        return -1;

    pthread_mutex_lock(&s->mutex);

    pthread_mutex_destroy(&s->mutex);
    pthread_cond_destroy(&s->cond);

    pthread_mutex_unlock(&s->mutex);

    return 1;
}