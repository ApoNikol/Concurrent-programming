#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h> // For time()

typedef struct {
    pthread_mutex_t mtx;
    pthread_cond_t work_ready;
    pthread_cond_t workers_available;
    volatile int *interconnection;
    volatile long long int *work_num;
    int num_workers;
    int av_workers;
} Monitor;

Monitor monitor;

void init(Monitor *mon, int num_workers) {
    pthread_mutex_init(&mon->mtx, NULL);
    pthread_cond_init(&mon->work_ready, NULL);
    pthread_cond_init(&mon->workers_available, NULL);
    mon->interconnection = calloc(num_workers, sizeof(int));
    mon->work_num = calloc(num_workers, sizeof(long long int));
    mon->num_workers = num_workers;
    mon->av_workers = num_workers;
}

void destroy(Monitor *mon) {
    pthread_mutex_destroy(&mon->mtx);
    pthread_cond_destroy(&mon->work_ready);
    pthread_cond_destroy(&mon->workers_available);
    free((void *)mon->interconnection);
    free((void *)mon->work_num);
}

void assign_work(Monitor *mon, long long int num) {
    pthread_mutex_lock(&mon->mtx);
    if(mon->av_workers == 0) 
        pthread_cond_wait(&mon->workers_available, &mon->mtx);

    for (int i = 0; i < mon->num_workers; i++) {
        if (!mon->interconnection[i]) {
            mon->work_num[i] = num;
            mon->interconnection[i] = 1;
            mon->av_workers--;
            pthread_cond_signal(&mon->work_ready);
            break;
        }
    }
    pthread_mutex_unlock(&mon->mtx);
}

void get_work(Monitor *mon, int id) {
    pthread_mutex_lock(&mon->mtx);
    while (mon->interconnection[id] == 0) {
        //printf("worker %d sleeps\n", id);
        pthread_cond_wait(&mon->work_ready, &mon->mtx);
        //printf("worker %d\n", id);
        if(mon->interconnection[id] == 0 || mon->interconnection[id] == -1) 
            pthread_cond_signal(&mon->work_ready);
    }
    pthread_mutex_unlock(&mon->mtx);
}

void worker_done(Monitor *mon, int id) {
    pthread_mutex_lock(&mon->mtx);
    if(mon->interconnection[id] != -1)
        mon->interconnection[id] = 0;
    mon->av_workers++;
    pthread_cond_signal(&mon->workers_available);
    pthread_mutex_unlock(&mon->mtx);
}

void terminate_threads(Monitor *mon) {
    pthread_mutex_lock(&mon->mtx);
    for (int i = 0; i < mon->num_workers; i++)
        mon->interconnection[i] = -1;
    pthread_cond_signal(&mon->work_ready);
    pthread_mutex_unlock(&mon->mtx);
}

int is_prime(long long int num) {
    if (num <= 1)
        return 0; // Numbers less than 2 are not prime
    if (num <= 3)
        return 1; // 2 and 3 are prime numbers

    // Eliminate multiples of 2 and 3
    if (num % 2 == 0 || num % 3 == 0)
        return 0;

    // Check for factors from 5 to the square root of num
    for (long long int i = 5; i * i <= num; i += 6)
    {
        if (num % i == 0 || num % (i + 2) == 0)
        {
            return 0; // Found a divisor
        }
    }

    return 1; // No divisors found, it's prime
}


void *routine(void *arg) {
    long int i = *(int *)arg;
    free(arg);

    while (1) {  
        get_work(&monitor, i);

        if(monitor.interconnection[i] == -1) 
            break;

        if (is_prime(monitor.work_num[i]))
            printf("\nNumber %lld is prime\n", monitor.work_num[i]);
        else
            printf("\nNumber %lld is not prime\n", monitor.work_num[i]);

        worker_done(&monitor, i);  
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    FILE *file = NULL;
    long long int num;
    int i;
    char *input = NULL;
    size_t len = 0;
    ssize_t nread;

    // Get the start time
    struct timeval tic, toc;
    gettimeofday(&tic, NULL);
   
    if (argc != 2 && argc != 3) {
        printf("Usage: %s <num_workers> [binary_file]\n", argv[0]);
        return -1;
    }

    long int num_workers = atol(argv[1]);
    pthread_t thr[num_workers];

    init(&monitor, num_workers);
    
    // If a file is provided, open it
    if (argc == 3) {
        file = fopen(argv[2], "rb");
        if (file == NULL) {
            printf("Error: Unable to open file %s\n", argv[2]);
            return -1;
        }
        printf("Reading numbers from file: %s\n", argv[2]);
    }
    else
        printf("No file provided, entering interactive mode. Type numbers:\n");

    // Create threads
    for (i = 0; i < num_workers; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        pthread_create(&thr[i], NULL, routine, thread_id);
    }

    // Input from file or stdin
    while (1) {
        if (file) {
            // Read long long int from the binary file
            if (fread(&num, sizeof(long long int), 1, file) != 1)
            {
                printf("End of file\n");
                break; // EOF or read error
            }
            //printf("\nRead number %lld from file\n", num);
        }
        else {
            // Read long long int from user input
            nread = getline(&input, &len, stdin);
            if (nread == -1) {
                printf("End of file given as input\n");
                break; // End of input (Ctrl+D)
            }
            num = atoll(input); // Convert input to long long int
            printf("\nUser entered %lld\n", num);
        }

        assign_work(&monitor, num);
    }

    terminate_threads(&monitor);

    // Wait for threads to finish
    for (i = 0; i < num_workers; i++)
        pthread_join(thr[i], NULL);

    printf("Threads closed\n");

    // Get the end time
    gettimeofday(&toc, NULL);

    // Calculate and print the total elapsed time in seconds
    double time_spent = (double) (toc.tv_usec - tic.tv_usec) / 1000000 + (double) (toc.tv_sec - tic.tv_sec);
    printf("Total elapsed time: %.3f seconds\n", time_spent);

    destroy(&monitor);
    if (file)
        fclose(file);
    free(input);

    return 0;
}