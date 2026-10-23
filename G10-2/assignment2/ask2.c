#include "semaph_lib.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h> // For time()

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

volatile int *interconnection = NULL;
volatile long long int *work_num = NULL;
mysem_t *worksem = NULL;

int av_workers;
mysem_t main_sleep;
mysem_t mutex;

void *routine(void *arg) {
    long int i = *(int *)arg;
    free(arg);
    int res;

    while (1) {  
        res = mysem_down(&worksem[i]); // LOCK THE WORKER
        if(res == -1)   
            printf("Invalid down\n");

        if (interconnection[i] == -1)
            break;

        res = is_prime(work_num[i]);
        if (res)
            printf("\nNumber %lld is prime\n", work_num[i]);
        else
            printf("\nNumber %lld is not prime\n", work_num[i]);
        if(interconnection[i] != -1) 
            interconnection[i] = 0;
        
        mysem_down(&mutex);
        av_workers++;
        if(av_workers == 0)
            mysem_up(&main_sleep);
        mysem_up(&mutex);
  
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    FILE *file = NULL;
    long long int num;
    int i;
    int res;
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


    interconnection = (int *)calloc(num_workers, sizeof(int));
    work_num = (long long int *)calloc(num_workers, sizeof(long long int));
    worksem = (mysem_t *)malloc(num_workers * sizeof(mysem_t));
    av_workers = num_workers;
    mysem_init(&main_sleep,0);
    mysem_init(&mutex,1);
    
    for(i = 0; i < num_workers; i++) {
        worksem[i].initialized = 0;
    }
    
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
        mysem_init(&worksem[i], 0);
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
            printf("\nRead number %lld from file\n", num);
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
        mysem_down(&mutex);
        av_workers--;
        if(av_workers == -1) {
            mysem_up(&mutex);
            mysem_down(&main_sleep);
        }
        else    
            mysem_up(&mutex);

        for (int i = 0; i < num_workers; i++) {
            if (!interconnection[i]) {
                work_num[i] = num;
                interconnection[i] = 1;
                res = mysem_up(&worksem[i]);
                if(res == -1)
                    printf("Invalid up\n");
                break;
            }
        }
    }

    // Signal threads to exit
    for (i = 0; i < num_workers; i++){
        interconnection[i] = -1;
        res = mysem_up(&worksem[i]);}

    // Wait for threads to finish
    for (i = 0; i < num_workers; i++)
        pthread_join(thr[i], NULL);

    printf("Threads closed\n");

    // Get the end time
    gettimeofday(&toc, NULL);

    // Calculate and print the total elapsed time in seconds
    double time_spent = (double) (toc.tv_usec - tic.tv_usec) / 1000000 + (double) (toc.tv_sec - tic.tv_sec);
    printf("Total elapsed time: %.3f seconds\n", time_spent);

    // Clean up
    if (file)
        fclose(file);
    free((void *)interconnection);
    free((void *)work_num);
    free(input);

    mysem_destroy(&main_sleep);
    mysem_destroy(&mutex);
    for(i = 0; i < num_workers; i++) {
        mysem_destroy(&worksem[i]);
    }

    return 0;
}