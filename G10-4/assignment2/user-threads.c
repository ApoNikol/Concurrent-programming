#include <ucontext.h>
#include <stdio.h>
#include "coroutines.c"
#include <sys/time.h>
#include <signal.h>

typedef enum {
    BLOCKED,
    RUNNING,
    READY,
    SLEEPING
} thread_state_t;

typedef struct mythr_t {
    co_t ctx;
    thread_state_t state;
    struct mythr_t *next;
    struct mythr_t *prev;
    struct mythr_t **join;
    struct mythr_t *next_blocked; 
    int join_num;
    int id;
    int finished;
    double sleep_time;
} mythr_t;

typedef struct {
    int val;
    mythr_t *blocked_list;
} mysem_t;

int list_size;
mythr_t *thr_running;
struct itimerval timer;
struct itimerval timer2;

mythr_t **sleeping_threads = NULL;
int sleeping_count = 0;  // Number of threads in the array


void pause_timer() {
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGALRM); 
    sigprocmask(SIG_BLOCK, &block_set, NULL);
}

void resume_timer() {
    sigset_t unblock_set;
    sigemptyset(&unblock_set);
    sigaddset(&unblock_set, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &unblock_set, NULL);
}

void signal_handler(int signum) {  
    int found = 0;
    mythr_t *tmp = thr_running;

    pause_timer();

    printf("SIGARLM\n");
    if(thr_running->state == RUNNING) {
        thr_running->state = READY;
    }  
        
    while(1) {                              //endless loop if no thread is ready
        thr_running = thr_running->next;
        if(thr_running->state == READY) {
            thr_running->state = RUNNING;
            found = 1;
            break;
        }
    } 
    if(!found) {
        printf("All threads blocked, terminating.\n");
        exit(EXIT_FAILURE);
    }

    setitimer(ITIMER_REAL, &timer, NULL);

    resume_timer();

    mycoroutines_switchto(&tmp->ctx, &thr_running->ctx);
}

void signal_handler2(int sig) {

    pause_timer();

    for (int i = 0; i < sleeping_count; ) {
        mythr_t *thread = sleeping_threads[i];
        thread->sleep_time -= 1;

        if (thread->sleep_time <= 0) {
            thread->state = READY;  

            // Remove the thread from the sleeping array
            sleeping_threads[i] = sleeping_threads[sleeping_count - 1];
            sleeping_count--;
            sleeping_threads = (mythr_t **)realloc(sleeping_threads, sleeping_count * sizeof(mythr_t *));
            if (sleeping_count > 0 && sleeping_threads == NULL) {
                perror("Realloc failed");
                exit(1);
            }
        } 
        else 
            i++; 
    }

    resume_timer();
}


int mythreads_init(mythr_t *main) {   

    pause_timer(); 

    struct sigaction sa1;
    sa1.sa_handler = signal_handler;  
    sa1.sa_flags = SA_RESTART;                 
    sigemptyset(&sa1.sa_mask);  

    if (sigaction(SIGALRM, &sa1, NULL) == -1) {
        perror("Error setting signal handler");
        resume_timer();
        return -1;
    }

    timer.it_value.tv_sec = 2;    
    timer.it_value.tv_usec = 0;     // 5 us between thread switch       
    timer.it_interval.tv_sec = 0;      
    timer.it_interval.tv_usec = 0;    
    
    struct sigaction sa2;
    sa2.sa_handler = signal_handler2; 
    sa2.sa_flags = SA_RESTART;       
    sigemptyset(&sa2.sa_mask);      

    if (sigaction(SIGVTALRM, &sa2, NULL) == -1) {
        perror("sigaction");
        resume_timer();
        return -1;
    }

    timer2.it_value.tv_sec = 1;      //timer2.it_value.tv_usec = 100000;
    timer2.it_value.tv_usec = 0;
    timer2.it_interval.tv_sec = 1;
    timer2.it_interval.tv_usec = 0;  //timer2.it_interval.tv_usec = 100000; 
 
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("Error starting timer");
        resume_timer();
        return -1;
    }

    if (setitimer(ITIMER_VIRTUAL, &timer2, NULL) == -1) {
        perror("setitimer");
        resume_timer();
        return -1;
    }

    mycoroutines_init(&main->ctx);
    main->state = RUNNING;
    main->next = main;
    main->prev = main;
    main->join = NULL;
    main->next_blocked = NULL;
    main->join_num = 0;
    main->id = 0;
    main->finished = 0;
    main->sleep_time = 0;

    thr_running = main;
    list_size = 1;

    resume_timer();

    return 0;
}

int mythreads_create(mythr_t *thr, void (body)(void *), void *arg) {  

    pause_timer();

    thr->next = thr_running;
    thr->prev = thr_running->prev;
    thr->prev->next = thr;
    thr->next->prev = thr;
    
    mycoroutines_create(&thr->ctx, body, arg);
    thr->state = READY;
    thr->join = NULL;
    thr->next_blocked = NULL;
    thr->join_num = 0;
    thr->id = list_size;
    thr->finished = 0;
    thr->sleep_time = 0;

    list_size++;

    resume_timer();
    
    return 0;
}

int mythreads_yield() {      

    pause_timer();  

    thr_running->state = READY;
    if(list_size == 1) {
       printf("Current thread is the only thread active\n");
    }

    resume_timer(); 
    
    signal_handler(0);

    return 0;
}

// wake all joined threads due to this thread //
int mythreads_destroy(mythr_t *thr) {
    int i;

    pause_timer(); 

    thr->finished = 1; 
    for(i = 0; i < thr->join_num; i++) 
        thr->join[i]->state = READY;
    if(thr->join != NULL)
        free(thr->join);
    if(list_size == 1){
        printf("No other threads currently active\n");
    }
    
    resume_timer();

    return 0;
}

int mythreads_join(mythr_t *thr) {

    if (!thr->finished) {
        pause_timer();

        thr_running->state = BLOCKED;
        thr->join_num++;
        thr->join = realloc(thr_running->join, sizeof(mythr_t *) * thr_running->join_num);
        thr->join[thr_running->join_num - 1] = thr_running;

        resume_timer();

        signal_handler(0);
    }

    return 0;
}

int mythreads_sleep(int secs) {

    pause_timer();

    thr_running->state = SLEEPING;
    thr_running->sleep_time = secs;

    sleeping_count++;  
    sleeping_threads = (mythr_t **)realloc(sleeping_threads, sizeof(mythr_t *) * sleeping_count);
    sleeping_threads[sleeping_count - 1] = thr_running;

    resume_timer();

    signal_handler(0);

    return 0;
}

int mythreads_sem_create(mysem_t *s, int val) {

    pause_timer();

    if(val != 1 && val != 0) {
        printf("Invalid value.\n");
        resume_timer();
        return -1;
    }
    s->val = val;
    s->blocked_list = NULL;

    resume_timer();

    return 1;
}

int mythreads_sem_down(mysem_t *s) {

    pause_timer();

    mythr_t *curr;

    if(s->val) {
        s->val = 0;
        resume_timer();
    }
    else {
        thr_running->state = BLOCKED;
        
        if(s->blocked_list == NULL) {       //when list is empty
            s->blocked_list = thr_running;
            s->blocked_list->next_blocked = NULL;
        }
        else {
            curr = s->blocked_list;
            while(curr->next_blocked != NULL) //find last pos
                curr = curr->next_blocked;

            curr->next_blocked = thr_running;
            thr_running->next_blocked = NULL;
        }
        resume_timer();

        signal_handler(0);
    }
    return 0;
}

int mythreads_sem_up(mysem_t *s) {

    pause_timer();

    if(s->val) {
        printf("Invalid up.\n");
        resume_timer();
        return -1;
    }
    else if(s->blocked_list == NULL) //when list is empty
        s->val = 1;
    else {
        s->blocked_list->state = READY;
        s->blocked_list = s->blocked_list->next_blocked;
    }

    resume_timer();

    return 0;
}

int mythreads_sem_destroy(mysem_t *s) {
    if(s->blocked_list != NULL)
        printf("There are threads waiting\n");

    return 0;
}