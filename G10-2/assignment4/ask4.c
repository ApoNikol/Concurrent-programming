#include "semaph_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

mysem_t sleep_main,pq,train_sleep,pw_mutex,wake_train_mtx;
volatile int pw;
volatile int flag_main = 0;
volatile int wake_train = 0;

struct train{
    int max_passengers;
};

void *train_routine(void *argv){
    int ride = 0;
    int j=0;
    int max_passengers = *((int *)argv);
    //An h main den teleiwse h an yparxei kapoios passenger na perimenei
    while(!flag_main || pw){
        //An h oyra exei ligoterous apo max_passengers kai h main den exei teleiwsei, koimhsoy
        if(pw < max_passengers && !flag_main){
            printf("Train starts waiting...\n");
            //mutex for wake_train not for the value not to change
            mysem_down(&wake_train_mtx);
            wake_train = 1;
            mysem_up(&wake_train_mtx);
            //
            mysem_down(&train_sleep);
        }
        if(pw<max_passengers) //IN THE ONLY CASE THA MAIN GIVES CONTROL D AND WE HAVE LEAST AND LAST PASSENGERS
            ride =pw;
        else
            ride = max_passengers;
        printf("Train leaves.\n%d passengers are on the ride\n",ride);
        if(pw!=0)
        //train leaves
        sleep(5);

        mysem_down(&pw_mutex);
        pw -= ride;
        mysem_up(&pw_mutex);

        for(j=0;j<ride;j++)
            mysem_up(&pq);
    }
    //wake up main
    mysem_up(&sleep_main);
    return NULL;
}
    
void *passenger_routine(void *argv){
    int max_passengers = *((int *)argv);
    if(pw >= max_passengers && wake_train){
        mysem_down(&wake_train_mtx);
        wake_train = 0;
        mysem_up(&wake_train_mtx);
        mysem_up(&train_sleep);
    }
    mysem_down(&pw_mutex);
    pw++;
    mysem_up(&pw_mutex);
    mysem_down(&pq);
    printf("Thread got out of the train\n");
    //3ypnise apo treno

    return NULL;
}

int main(int argc, char *argv[]){
    struct train train;
    pthread_t thr_train;
    pthread_t *thr_passengers =NULL;
    int num_passengers = 0;
    int i =0;
    int res;
    int old_passengers = 0;
    int total_passengers = 0;
    
    res = mysem_init(&sleep_main, 0);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&pq, 0);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&pw_mutex, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    res = mysem_init(&train_sleep, 0);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    res = mysem_init(&wake_train_mtx, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    if(argc!=2){
        printf("Wrong number of arguments\n");
        return 0;
    }
    
    train.max_passengers = atoi(argv[1]);

    printf("Creating train...\n");
    pthread_create(&thr_train, NULL, train_routine, &train.max_passengers);
    
    while (scanf(" %d", &num_passengers) != EOF) {
        printf("Got num_passengers input %d\n",num_passengers);
        total_passengers+=num_passengers;
        thr_passengers = realloc(thr_passengers, total_passengers * sizeof(pthread_t));
        if (thr_passengers == NULL) {
            perror("Failed to reallocate memory for thr_passengers");
            exit(EXIT_FAILURE);
        }
        for(i = old_passengers ; i<total_passengers;i++){
            pthread_create(&thr_passengers[i],NULL,passenger_routine,&train.max_passengers);
            printf("Passenger num %d, wants to join.\n",i);
        }
        old_passengers = total_passengers;
        num_passengers = 0;
    }
    flag_main = 1;
    printf("Line closed.\nWaiting for train to load the remaining queue passengers\n");
    if(wake_train){
        wake_train = 0;
        mysem_up(&train_sleep);
    }
    mysem_down(&sleep_main);
    free(thr_passengers); 

    mysem_destroy(&sleep_main);
    mysem_destroy(&pq);
    mysem_destroy(&train_sleep);
    mysem_destroy(&pw_mutex);
    mysem_destroy(&wake_train_mtx);
    
    printf("Program closing\n");
    return 0 ;
}