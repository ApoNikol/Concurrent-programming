#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "pipelib.h"

typedef struct {
    char *data;
    int write;
    int read;
    int closed;
    int id;
    int buffer_size;
} RingBuffer;

typedef struct {
    RingBuffer *pipe_array;
    int size;
    int *deleted_id;
    int deleted_size;
} total_buffers;

volatile static total_buffers *tb = NULL;

void check_null(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
}

void initTotalBuffers() {
    tb = (total_buffers *)malloc(sizeof(total_buffers));
    check_null((total_buffers *)tb);
    
    tb->pipe_array = NULL;
    tb->size = 0;
    tb->deleted_id = NULL;
    tb->deleted_size = 0;
}

void delete_tb(int i) {
    int temp_id;
    
    if (tb == NULL) 
        initTotalBuffers();
  
    temp_id = tb->pipe_array[i].id;
    free(tb->pipe_array[i].data);
    
    tb->pipe_array[i] = tb->pipe_array[tb->size - 1];    //overwrite closed pipe
    tb->size--;
    tb->pipe_array = (RingBuffer *)realloc(tb->pipe_array, sizeof(RingBuffer) * (tb->size));
    if (tb->size)
        check_null(tb->pipe_array);

    tb->deleted_id = (int *)realloc(tb->deleted_id, sizeof(int)*(tb->deleted_size + 1));
    check_null(tb->deleted_id);
    
    tb->deleted_id[tb->deleted_size] = temp_id;
    tb->deleted_size++;

    if(tb->size == 0) {
        free((total_buffers *)tb);
    }    
}

void CreatePipe(RingBuffer *rb, int id, int buffer_size) {
    rb->data = (char *)malloc(sizeof(char)*buffer_size);
    check_null(rb->data);
    
    rb->write = 0;
    rb->read = 0;
    rb->closed = 0;
    rb->id = id;
    rb->buffer_size = buffer_size;
}

int generate_id(volatile total_buffers *tb) {
    int id;

    if (tb == NULL) 
        initTotalBuffers();

    if(tb->deleted_size) {  
        tb->deleted_size--;     
        id = tb->deleted_id[tb->deleted_size];  
        tb->deleted_id = (int *)realloc(tb->deleted_id, sizeof(int)*(tb->deleted_size));
        
        if(tb->deleted_size)
            check_null(tb->deleted_id);     
    }
    else 
        id = tb->size;                 // if there is no missing id, there should not be any id = tb->size
    
    return id;
}

int pipe_open(int size) {
    int id;

    if (tb == NULL) 
        initTotalBuffers();
    
    tb->pipe_array = (RingBuffer *)realloc(tb->pipe_array, sizeof(RingBuffer) * (tb->size + 1)); // realloc
    check_null(tb->pipe_array);
    tb->size++; 
    
    id = generate_id(tb);
    CreatePipe(&tb->pipe_array[tb->size-1], id, size);  
    
    return id;
}

int pipe_write(int p, char c) {
    int exists = 0;
    int i;
    RingBuffer *pipe;

    if (tb == NULL) 
        initTotalBuffers();
    
    for(i = 0; i < tb->size; i++) {
        if((p == tb->pipe_array[i].id) && (!tb->pipe_array[i].closed)) {
            exists = 1;
            break;
        }                                          
    } 
    
    if(!exists)
        return -1;
    
    pipe = &tb->pipe_array[i];
    
    while(((pipe->write + 1) % pipe->buffer_size) == pipe->read) 
        continue;
    
    pipe->data[pipe->write] = c;
    tb->pipe_array[i].write = (tb->pipe_array[i].write + 1) % tb->pipe_array[i].buffer_size;
    
    return 1;
}

int pipe_writeDone(int p) {
    int i, exists = 0;

    if (tb == NULL) 
        initTotalBuffers();
    for(i = 0; i < tb->size; i++) {
        if((p == tb->pipe_array[i].id) && (!tb->pipe_array[i].closed)) {  // if p matches an already existing id..
            exists = 1;
            break;
        }
    }
    
    if(!exists)
        return -1;
    
    tb->pipe_array[i].closed = 1;
    
    return 1;
}

int pipe_read(int p, char *c) {
    int exists = 0;
    int i;
    RingBuffer *pipe;

    if (tb == NULL) 
        initTotalBuffers();

    for(i = 0; i < tb->size; i++) {
        if(p == tb->pipe_array[i].id) {
            exists = 1;
            break;
        }  
    }
    
    if(!exists)
        return -1;       

    pipe = &tb->pipe_array[i];
    
    while(pipe->read == pipe->write) {
        if (pipe->closed) {    //closed pipe and read has read everything
            delete_tb(i);
            return 0;
        }    
    }
    
    *c = pipe->data[pipe->read];
    pipe->read = (pipe->read + 1) % pipe->buffer_size;
    
    return 1;
}