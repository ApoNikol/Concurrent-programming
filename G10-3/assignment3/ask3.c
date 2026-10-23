#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct car {
    int car_num;
    int team;
} car_t;

typedef enum {
    RED = 0,
    BLUE = 1,
    WHITE = -1
} Color;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t red_cond, blue_cond;
    int volatile rw, bw; 
    int volatile red_bridge, blue_bridge;
    int volatile flag;
    int volatile cars_passing;
    int cons_passed;
} BridgeMonitor;

BridgeMonitor monitor;

int bridge, threshold;

void bridge_init(BridgeMonitor *mon) {
    pthread_mutex_init(&mon->mutex, NULL);
    pthread_cond_init(&mon->red_cond, NULL);
    pthread_cond_init(&mon->blue_cond, NULL);
    mon->rw = 0;
    mon->bw = 0;
    mon->red_bridge = 0;
    mon->blue_bridge = 0;
    mon->flag = WHITE; 
}

void bridge_destroy(BridgeMonitor *mon) {
    pthread_mutex_destroy(&mon->mutex);
    pthread_cond_destroy(&mon->red_cond);
    pthread_cond_destroy(&mon->blue_cond);
}

void enter_bridge_red(BridgeMonitor *mon, struct car current_car) {
    pthread_mutex_lock(&mon->mutex);

    mon->cars_passing++;
    if(mon->flag == WHITE)
        mon->flag = RED;

    if(mon->red_bridge == bridge || mon->flag == BLUE || ((mon->cons_passed >= threshold) && (mon->flag == RED))) {
        mon->rw++;
        
        printf("car:%d team:RED SLEEPING...\n", current_car.car_num);
        pthread_cond_wait(&mon->red_cond, &mon->mutex); //wait
        printf("car:%d team:RED WOKE UP...\n", current_car.car_num);

        printf("red_bridge: %d\n", mon->red_bridge);
        mon->cons_passed++;
        if(mon->rw > 0 && mon->red_bridge < bridge && mon->cons_passed < threshold) {
            mon->rw--;
            mon->red_bridge++;
            pthread_cond_signal(&mon->red_cond);            
        }
    }
    else {
        mon->red_bridge++;
        mon->cons_passed++;
    }
    printf("Threshold is: %d\n", mon->cons_passed);
    pthread_mutex_unlock(&mon->mutex);
}

void enter_bridge_blue(BridgeMonitor *mon, struct car current_car) {
    pthread_mutex_lock(&mon->mutex);

    mon->cars_passing++;
    if(mon->flag == WHITE)
        mon->flag = BLUE;

    if(mon->blue_bridge == bridge || mon->flag == RED || ((mon->cons_passed >= threshold) && (mon->flag == BLUE))) {
        mon->bw++;
        
        printf("car:%d team:BLUE SLEEPING...\n", current_car.car_num);
        pthread_cond_wait(&mon->blue_cond, &mon->mutex); //wait
        printf("car:%d team:BLUE WOKE UP...\n", current_car.car_num);

        printf("blue_bridge: %d\n", mon->blue_bridge);
        mon->cons_passed++;
        if(mon->bw > 0 && mon->blue_bridge < bridge && threshold) {
            mon->bw--;
            mon->blue_bridge++;
            pthread_cond_signal(&mon->blue_cond);            
        }
    }
    else {
        mon->blue_bridge++;
        mon->cons_passed++;
    }
    printf("Threshold is: %d\n", mon->cons_passed);
    pthread_mutex_unlock(&mon->mutex);
}

void grant_priority_check(BridgeMonitor *mon, struct car current_car,
  int *grant_priority) {

    pthread_mutex_lock(&mon->mutex);
    
    if(current_car.team == RED) {
        if(mon->bw != 0) {
            if((mon->rw / mon->bw) >= 10 || mon->cons_passed >= threshold)
                *grant_priority = 1;
            else
                *grant_priority = 0;
        }
        else 
            *grant_priority = 0;
    }
    else {
        if(mon->rw != 0) {
            if((mon->bw / mon->rw) >= 10)
                *grant_priority = 1;
            else 
                *grant_priority = 0;
        }
        else 
            *grant_priority = 0;
    }

    pthread_mutex_unlock(&mon->mutex);
}

void exit_bridge_red(BridgeMonitor *mon, struct car current_car, int grant_priority) {

    pthread_mutex_lock(&mon->mutex);

    if((mon->red_bridge == 1 || grant_priority)) {        
        mon->red_bridge--; // car exit

        if(mon->red_bridge)  
            printf("car:%d team:RED CROSSED THE BRIDGE, PRIORITY WILL BE GIVEN TO BLUE. %d\n", current_car.car_num, mon->red_bridge);
        else if(mon->bw > 0) {  //last car on bridge and car on the other side
            mon->flag = BLUE;  // flag given
            mon->cons_passed = 0;
            printf("car:%d team:RED CROSSED THE BRIDGE, PRIORITY GIVEN TO BLUE. %d\n", current_car.car_num, mon->red_bridge);
            mon->blue_bridge++; /// increment blue bridge before waking up any car 
            mon->bw--; 
            pthread_cond_signal(&mon->blue_cond);
        }
    }
    else {
        mon->red_bridge--; // car exit

        printf("car:%d team:RED CROSSED THE BRIDGE. %d\n", current_car.car_num, mon->rw);

        if(mon->rw > 0 && mon->red_bridge < bridge && mon->cons_passed < threshold) {
            mon->rw--;
            mon->red_bridge++;

            pthread_cond_signal(&mon->red_cond);
        }
    } 

    mon->cars_passing--;
    if(!mon->cars_passing) {
        printf("\nAll cars exited. Setting flag to White\n\n");
        mon->flag = WHITE;
        mon->cons_passed = 0;
    }

    pthread_mutex_unlock(&mon->mutex);
}

void exit_bridge_blue(BridgeMonitor *mon, struct car current_car, int grant_priority) {

    pthread_mutex_lock(&mon->mutex);

    if((mon->blue_bridge == 1 || grant_priority)) {        
        mon->blue_bridge--; // car exit

        if(mon->blue_bridge)  
            printf("car:%d team:BLUE CROSSED THE BRIDGE, PRIORITY WILL BE GIVEN TO RED. %d\n", current_car.car_num, mon->blue_bridge);
        else if(mon->rw > 0) {  //last car on bridge and car on the other side
            mon->flag = RED;  // flag given
            mon->cons_passed = 0;
            printf("car:%d team:BLUE CROSSED THE BRIDGE, PRIORITY GIVEN TO RED. %d\n", current_car.car_num, mon->blue_bridge);
            mon->red_bridge++; /// increment blue bridge before waking up any car 
            mon->rw--; 
            pthread_cond_signal(&mon->red_cond);
        }
    }
    else {
        mon->blue_bridge--; // car exit

        printf("car:%d team:BLUE CROSSED THE BRIDGE. %d\n", current_car.car_num, mon->bw);

        if(mon->bw > 0 && mon->blue_bridge < bridge && mon->cons_passed < threshold) {
            mon->bw--;
            mon->blue_bridge++;

            pthread_cond_signal(&mon->blue_cond);
        }
    } 

    mon->cars_passing--;
    if(!mon->cars_passing) {
        printf("\nCars exited. Setting flag to White\n\n");
        mon->flag = WHITE;
        mon->cons_passed = 0;
    }

    pthread_mutex_unlock(&mon->mutex);
}


void *routine(void *arg) {
    struct car current_car = *(struct car *)arg; 
    int grant_priority;

    if(current_car.team == RED) {
        enter_bridge_red(&monitor, current_car);

        printf("car:%d team:RED BRIDGE CROSSING.\n", current_car.car_num);

        sleep(2);                    // crossing bridge

        grant_priority_check(&monitor, current_car, &grant_priority);

        // exit bridge code 
        exit_bridge_red(&monitor, current_car, grant_priority); 
    }
    /////////////////////////////////////////////////////////////////////
    else if(current_car.team == BLUE) {
        enter_bridge_blue(&monitor, current_car);

        printf("car:%d team:BLUE BRIDGE CROSSING.\n", current_car.car_num);

        sleep(2);                 // crossing bridge

        grant_priority_check(&monitor, current_car, &grant_priority);

        // exit bridge code
        exit_bridge_blue(&monitor, current_car, grant_priority); 
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    int i, num_cars;
    int cars_number = 0;
    pthread_t *thr = NULL;
    car_t *cars = NULL;

    if(argc != 3) {
        printf("Wrong number of arguments!\n");
        return 1;
    }

    bridge_init(&monitor);
    
    bridge = atoi(argv[1]);

    threshold = atoi(argv[2]);

    while (scanf(" %d", &num_cars) != EOF) {
        printf("\nSpawning %d cars.\n\n",num_cars);
        cars_number += num_cars;

        thr = realloc(thr, cars_number * sizeof(pthread_t));
        if (thr == NULL) {
            perror("Failed to reallocate memory for thr_passengers\n\n\n");
            exit(EXIT_FAILURE);
        }
        cars = realloc(cars, sizeof(struct car) * cars_number);
        if (thr == NULL) {
            perror("Failed to reallocate memory for thr_passengers\n\n\n");
            exit(EXIT_FAILURE);
        }

        for(i = (cars_number - num_cars); i < cars_number; i++) {
            cars[i].car_num = i + 1;
            //cars[i].team = rand() % 2;
            if(i < cars_number - 60)  // ./ask3 10 50 .. enter 120
                cars[i].team = 0;
            else
                cars[i].team = 1;
            //printf("Car number:%d of team:%d\n", cars[i].car_num, cars[i].team);
            pthread_create(&thr[i], NULL, routine, &cars[i]);
        }
    }

    for (i = 0; i < cars_number; i++)
        pthread_join(thr[i], NULL);

    free(thr); 
    free(cars);
  
    printf("All cars passed!\n");

    bridge_destroy(&monitor);
    
    return 0;
}