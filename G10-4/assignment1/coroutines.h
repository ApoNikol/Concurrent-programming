#ifndef MYCOROUTINES_H
#define MYCOROUTINES_H

#include <ucontext.h>

// Define the coroutine structure
typedef struct co {
    ucontext_t ctx;     // The context of the coroutine
    char *stack;        // The stack used by the coroutine
} co_t;

int mycoroutines_init(co_t *main);
int mycoroutines_create(co_t *co, void (body)(void *), void *arg);
int mycoroutines_switchto(co_t *oldctx, co_t *newctx);
int mycoroutines_destroy(co_t *co);

#endif // MYCOROUTINES_H
