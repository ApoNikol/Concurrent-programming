#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ucontext.h> 
#include <signal.h>

typedef struct co {
    ucontext_t ctx;                
    char *stack;            
} co_t;

co_t *main_context;

int mycoroutines_init(co_t *main) {
    if (getcontext(&main->ctx) == -1) {
        perror("getcontext");
        return -1; 
    }
    main->stack = NULL; 

    main_context = main;

    return 0; 
}

int mycoroutines_create(co_t *co, void (body)(void *), void *arg) {
    if (getcontext(&co->ctx) == -1) 
        return -1;  

    co->stack = malloc(SIGSTKSZ);
    if (co->stack == NULL) 
        return -1;          

    co->ctx.uc_stack.ss_sp = co->stack;
    co->ctx.uc_stack.ss_size = SIGSTKSZ;
    co->ctx.uc_link = &main_context->ctx;            

    makecontext(&co->ctx, (void (*)())body, 1, arg); 

    return 0;
}

int mycoroutines_switchto(co_t *oldctx, co_t *newctx) {
    printf("switching.\n");

    if (swapcontext(&oldctx->ctx ,&newctx->ctx) == -1) 
        return -1; 

    return 0;
}

int mycoroutines_destroy(co_t *co) {
    if (co->ctx.uc_stack.ss_sp != NULL) {
        free(co->ctx.uc_stack.ss_sp);
    }

    return 0;
}
