#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "semaph_lib.h"

struct car {
    int car_num;
    int team;
    int left;
};

typedef enum {
    RED = 0,
    BLUE = 1,
    WHITE = -1
} Color;

mysem_t rq, bq, mutex, red_mtx1, blue_mtx1, red_mtx2, blue_mtx2, red_mtx, blue_mtx;
int sem_num = 0;
int cars_number;
int bridge;
int volatile cars_passed = 0;
int volatile rw = 0;
double ratio = 0;
int volatile bw = 0;
int volatile red_bridge = 0;
int volatile blue_bridge = 0;
int volatile flag = 0;


void *routine(void *arg) {
    struct car current_car = *(struct car *)arg; 
    int res;

    if(current_car.team == RED) {
        mysem_down(&mutex);
        if(flag == WHITE)
            flag = RED;
        mysem_up(&mutex);

        //dont check red bridge before previous one enters and increments red_bridge
        mysem_down(&red_mtx); 
        if(red_bridge == bridge || flag == BLUE) {
            mysem_down(&red_mtx2);
            rw++;
            mysem_up(&red_mtx2);
            
            printf("car:%d team:RED SLEEPING...\n", current_car.car_num);
            mysem_up(&red_mtx);
            mysem_down(&rq);
            printf("car:%d team:RED WOKE UP...\n", current_car.car_num);

            printf("red_bridge: %d\n", red_bridge);
            if(rw > 0 && red_bridge < bridge) {
                mysem_down(&red_mtx2);
                rw--;
                mysem_up(&red_mtx2);
                mysem_down(&red_mtx1);
                red_bridge++;
                mysem_up(&red_mtx1);
                mysem_up(&rq);
            }
            else {
                mysem_up(&red_mtx);
            }
        }
        else {
            mysem_down(&red_mtx1);
            red_bridge++;
            mysem_up(&red_mtx1);  // let next one pass (car entered bridge)
            mysem_up(&red_mtx);
        }

        printf("car:%d team:RED BRIDGE CROSSING.\n", current_car.car_num);

        sleep(2);                 // crossing bridge

        mysem_down(&blue_mtx2);
        if(bw != 0) {
            ratio = (rw / bw);
            mysem_up(&blue_mtx2);
        }
        else {
            mysem_up(&blue_mtx2);
            ratio = 0;
        }

        // exit bridge code
        mysem_down(&blue_mtx);
        if((red_bridge == 1 || ratio >= 10/1)) {        
            flag = BLUE;  // flag given
            mysem_down(&red_mtx1);
            red_bridge--; // car exit
            mysem_up(&red_mtx1);  

            if(red_bridge)  {
                printf("car:%d team:RED CROSSED THE BRIDGE, PRIORITY WILL BE GIVEN TO BLUE. %d\n", current_car.car_num, red_bridge);
                mysem_up(&blue_mtx);
            }
            else { //last car on bridge
                printf("car:%d team:RED CROSSED THE BRIDGE, PRIORITY GIVEN TO BLUE. %d\n", current_car.car_num, red_bridge);

                if(bw > 0) {     
                    mysem_down(&red_mtx1);   
                    blue_bridge++; /// increment blue bridge before waking up any car 
                    mysem_up(&red_mtx1);
                    mysem_down(&blue_mtx2);
                    bw--; 
                    mysem_up(&blue_mtx2);
                    res = mysem_up(&bq);
                    if(res == 0)
                        printf("Invalid up.\n");
                }
                else 
                    mysem_up(&blue_mtx);  
            }
        }
        else {
            mysem_down(&red_mtx1);
            red_bridge--; // car exit
            mysem_up(&red_mtx1);  

            printf("car:%d team:RED CROSSED THE BRIDGE. %d\n", current_car.car_num, rw);

            if(rw > 0 && red_bridge < bridge) {
                mysem_down(&red_mtx2);
                rw--;
                mysem_up(&red_mtx2);  

                mysem_down(&red_mtx1);
                red_bridge++;
                mysem_up(&red_mtx1);  // let next one pass (car exit)
                mysem_up(&rq);
            }
            mysem_up(&blue_mtx);
        }      
    }


///////////////////////////////////////////////////////////////////////////////////////////////////////


    else if(current_car.team == BLUE) {
        mysem_down(&mutex);
        if(flag == WHITE)
            flag = BLUE;
        mysem_up(&mutex);

        //dont check blue bridge before previous one enters and increments blue_bridge
        mysem_down(&blue_mtx); 
        if(blue_bridge == bridge || flag == RED) {

            mysem_down(&blue_mtx2);
            bw++;
            mysem_up(&blue_mtx2);
            
            printf("car:%d team:BLUE SLEEPING...\n", current_car.car_num);
            mysem_up(&blue_mtx);
            mysem_down(&bq);
            printf("car:%d team:BLUE WOKE UP...\n", current_car.car_num);

            printf("blue_bridge: %d\n", blue_bridge);
            if(bw > 0 && blue_bridge < bridge) {
                mysem_down(&blue_mtx2);
                bw--;
                mysem_up(&blue_mtx2);
                mysem_down(&blue_mtx1);
                blue_bridge++;
                mysem_up(&blue_mtx1);
                mysem_up(&bq);
            }
            else {
                mysem_up(&blue_mtx); 
            }
        }
        else {
            mysem_down(&blue_mtx1);
            blue_bridge++;
            mysem_up(&blue_mtx1);  // let next one pass (car entered bridge)
            mysem_up(&blue_mtx);
        }


        printf("car:%d team:BLUE BRIDGE CROSSING.\n", current_car.car_num);

        sleep(2);                 // crossing bridge

        mysem_down(&red_mtx2);
        if(rw != 0) {
            ratio = (bw / rw);
            mysem_up(&red_mtx2);
        }
        else {
            mysem_up(&red_mtx2);
            ratio = 0;
        }

        // exit bridge code
        mysem_down(&red_mtx);
        if((blue_bridge == 1 || ratio >= 10/1)) {        
            flag = RED;  // flag given
            mysem_down(&blue_mtx1);
            blue_bridge--; // car exit
            mysem_up(&blue_mtx1);  

            if(blue_bridge)  {
                printf("car:%d team:BLUE CROSSED THE BRIDGE, PRIORITY WILL BE GIVEN TO RED. %d\n", current_car.car_num, red_bridge);
                mysem_up(&red_mtx);
            }
            else { //last car on bridge
                printf("car:%d team:BLUE CROSSED THE BRIDGE, PRIORITY GIVEN TO RED. %d\n", current_car.car_num, red_bridge);
                if(rw > 0) {     
                    mysem_down(&blue_mtx1);   
                    red_bridge++; /// increment blue bridge before waking up any car 
                    mysem_up(&blue_mtx1);
                    mysem_down(&red_mtx2);
                    rw--; 
                    mysem_up(&red_mtx2);
                    res = mysem_up(&rq);
                    if(res == 0)
                        printf("Invalid up.\n");
                }
                else
                    mysem_up(&red_mtx);   
            }
        }
        else {
            mysem_down(&blue_mtx1);
            blue_bridge--; // car exit
            mysem_up(&blue_mtx1);  

            printf("car:%d team:BLUE CROSSED THE BRIDGE. %d\n", current_car.car_num, rw);

            if(bw > 0 && blue_bridge < bridge) {
                mysem_down(&blue_mtx2);
                bw--;
                mysem_up(&blue_mtx2);  

                mysem_down(&blue_mtx1);
                blue_bridge++;
                mysem_up(&blue_mtx1);  // let next one pass (car exit)

                mysem_up(&bq);
            }
            mysem_up(&red_mtx);
        }      
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    struct car *cars;
    int i;
    int res;

    if(argc != 3) {
        printf("Wrong number of arguments!\n");
        return 1;
    }
    
    res = mysem_init(&rq, 0);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&bq, 0);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&mutex, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&blue_mtx2, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");

    res = mysem_init(&blue_mtx1, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");    
    
    res = mysem_init(&red_mtx2, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&red_mtx1, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&red_mtx, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    
    res = mysem_init(&blue_mtx, 1);
    if(res == -1) 
        printf("mysem_init: FAILED\n");
    

    cars_number = atoi(argv[1]);
    bridge = atoi(argv[2]);

    pthread_t thr[cars_number]; 

    cars = (struct car *)malloc(sizeof(struct car) * cars_number);
    for (i = 0; i < cars_number; i++) {
        cars[i].car_num = i + 1;
        cars[i].left = 0;
        cars[i].team = rand() % 2;
        // if(i < 10)//190
        //     cars[i].team = 0;
        // else
        //     cars[i].team = 1;
        //printf("Car number:%d of team:%d\n", cars[i].car_num, cars[i].team);
        pthread_create(&thr[i], NULL, routine, &cars[i]);
    }

    for (i = 0; i < cars_number; i++)
        pthread_join(thr[i], NULL);
  
    printf("All cars passed!\n");
    mysem_destroy(&rq);
    mysem_destroy(&bq);
    mysem_destroy(&red_mtx1);
    mysem_destroy(&red_mtx2);
    mysem_destroy(&blue_mtx1);
    mysem_destroy(&blue_mtx2);
    mysem_destroy(&red_mtx);
    mysem_destroy(&blue_mtx);
    mysem_destroy(&mutex);
    
    return 0;
}