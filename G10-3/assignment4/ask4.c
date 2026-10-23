#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t train_cond;
    pthread_cond_t passenger_cond;
    volatile int pw; // Number of passengers waiting
    volatile int flag_main; // Main thread flag
} TrainMonitor;

void init(TrainMonitor *monitor) {
    pthread_mutex_init(&monitor->mutex, NULL);
    pthread_cond_init(&monitor->train_cond, NULL);
    pthread_cond_init(&monitor->passenger_cond, NULL);
    monitor->pw = 0;
    monitor->flag_main = 0;
}

void destroy(TrainMonitor *monitor) {
    pthread_mutex_destroy(&monitor->mutex);
    pthread_cond_destroy(&monitor->train_cond);
    pthread_cond_destroy(&monitor->passenger_cond);
}

TrainMonitor monitor;
int max_passengers;

void train_wait(TrainMonitor *mon, int *passengers) {
    pthread_mutex_lock(&mon->mutex);

    if(mon->pw < max_passengers && !mon->flag_main){
        printf("Train starts waiting...\n");
        pthread_cond_wait(&mon->train_cond, &mon->mutex);
    }

    if(mon->pw < max_passengers) //CNTRL D
        *passengers = mon->pw; 
    else
        *passengers = max_passengers;

    pthread_mutex_unlock(&mon->mutex);
}

void export_passengers(TrainMonitor *mon, int passengers) {
    pthread_mutex_lock(&mon->mutex);

    mon->pw -= passengers;
    for(int i = 0; i < passengers; i++)
        pthread_cond_signal(&mon->passenger_cond);

    pthread_mutex_unlock(&mon->mutex);
}

void wake_train(TrainMonitor *mon) {
    pthread_mutex_lock(&mon->mutex);

    mon->pw++;
    if(mon->pw >= max_passengers)
        pthread_cond_signal(&mon->train_cond);

    pthread_mutex_unlock(&mon->mutex);
}

void passenger_wait(TrainMonitor *mon, int id) {
    pthread_mutex_lock(&mon->mutex);

    pthread_cond_wait(&mon->passenger_cond, &mon->mutex);
    printf("Thread %d got out of the train\n", id);
    pthread_mutex_unlock(&mon->mutex); 
}

void *train_routine(void *argv){
    int passengers;

    while(!monitor.flag_main || monitor.pw){
        //train wait.
        train_wait(&monitor, &passengers);

        //begin ride
        if(passengers) {
            printf("\nTrain leaves.\n%d passengers are on the ride\n\n", passengers);
            sleep(5);
            printf("\nExporting passengers...\n\n");      
        }
        //export passenger
        export_passengers(&monitor, passengers);
    }
    return NULL;
}
    
void *passenger_routine(void *argv){
    int id = *((int *)argv);

    //wake the train
    wake_train(&monitor);

    //passenger wait.
    passenger_wait(&monitor, id);

    free(argv);
    return NULL;
}

int main(int argc, char *argv[]){
    pthread_t thr_train;
    pthread_t *thr_passengers = NULL;
    int num_passengers = 0;
    int i =0;
    int old_passengers = 0;
    int total_passengers = 0;
      
    if(argc!=2){
        printf("Wrong number of arguments\n");
        return 0;
    }
    
    max_passengers = atoi(argv[1]);

    init(&monitor);

    printf("\nCreating train...\n\n");
    pthread_create(&thr_train, NULL, train_routine, NULL);
    
    while (scanf(" %d", &num_passengers) != EOF) {
        printf("Got num_passengers input %d\n",num_passengers);
        total_passengers+=num_passengers;
        thr_passengers = realloc(thr_passengers, total_passengers * sizeof(pthread_t));
        if (thr_passengers == NULL) {
            perror("Failed to reallocate memory for thr_passengers");
            exit(EXIT_FAILURE);
        }
        for(i = old_passengers ; i<total_passengers;i++){
            int *thread_id = malloc(sizeof(int));
            *thread_id = i;
            pthread_create(&thr_passengers[i], NULL, passenger_routine, thread_id);
            printf("Passenger num %d, wants to join.\n",i);
        }
        old_passengers = total_passengers;
        num_passengers = 0;
    }
    monitor.flag_main = 1;
    pthread_cond_signal(&monitor.train_cond);
    printf("\nLine closed.\nWaiting for train to load the remaining queue passengers\n\n");

    for (i = 0; i < total_passengers; i++)
        pthread_join(thr_passengers[i], NULL);
    pthread_join(thr_train, NULL);
    free(thr_passengers); 

    printf("Program closing\n");

    destroy(&monitor);

    return 0 ;
}
