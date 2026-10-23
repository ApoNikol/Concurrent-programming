#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pipelib.h"      
#include "coroutines.h"

#define PIPE_SIZE 64

int read_done;
int write_done;
co_t main_ctx, read_ctx, write_ctx;

typedef struct {
    int id;
    char *FileRead;
} thread_data;

void routine_write(void *argv) {
    thread_data data = *(thread_data *)argv;
    FILE *FileName;                      
    char character;
    int res;
    int char_count = 0;


    FileName = fopen(data.FileRead, "r+");

    if (FileName == NULL) {
        printf("Invalid file.\n");
        exit(1);
    }
    
    while(fread(&character, 1, sizeof(char), FileName)) {   
        if(char_count >= PIPE_SIZE - 1) {                   //-1 because of the extra empty cell 
            printf("wrote %d bytes\n", char_count);
            char_count = 0; 
            mycoroutines_switchto(&write_ctx, &read_ctx);    
            printf("Continuing writing.\n");
        }
        char_count++;
        res = pipe_write(data.id, character); 
        if(res == -1)
            return ;    
    }
    
    res = pipe_writeDone(data.id);
    if(res == -1) {
        printf("pipe_writeDone failed.");
        return ;      
    } 

    printf("Closed write\n");
    write_done = 1;

    mycoroutines_switchto(&write_ctx ,&read_ctx); // read remaining bytes

    return ;
}

void routine_read(void *argv) {
    thread_data data = *(thread_data *)argv;
    FILE *FileName;                      
    char character;
    int res;
    int new_length;
    char *new_filename;
    char *dot;
    int char_count = 0;
    
    dot = strrchr(data.FileRead, '.');
    new_length = (dot - data.FileRead) + strlen(".copy") + 1;
    new_filename = (char *)malloc(new_length);
    
    if (new_filename == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);
    }
    
    strncpy(new_filename, data.FileRead, dot - data.FileRead);
    new_filename[dot - data.FileRead] = '\0';
    strcat(new_filename, ".copy");
    

    FileName = fopen(new_filename, "w+");
    free(new_filename);

    while(1) {
        if(char_count >= PIPE_SIZE - 1) {              // -1 because of the extra empty cell 
            printf("read %d bytes\n", char_count);
            char_count = 0;
            mycoroutines_switchto(&read_ctx ,&write_ctx);
            printf("Continuing reading.\n");
        }
        char_count++;
        res = pipe_read(data.id, &character);
        if(res != 1)
            break;         
        res = fwrite(&character, 1, sizeof(char), FileName);           
    }   
    if(res == -1) {
        printf("No pipe with this id.\n");
        fclose(FileName);
        read_done = 1;
        return ;    
    }
    else {
        printf("Closed read\n");
        fclose(FileName);
        read_done = 1;
    }

    return ;
}


int main(int argc, char *argv[]) {
    int res1, res2;
    thread_data *arguments;

    if(argc != 2) {
        printf("Wrong number of arguments\n");
        return -1;
    }

    arguments = (thread_data *)malloc(sizeof(thread_data));
    arguments->id = pipe_open(PIPE_SIZE);
    printf("First pipe opened at index: %d\n", arguments->id);

    arguments->FileRead = (char *)malloc(sizeof(char) * (strlen(argv[1]) + 1));
    strcpy(arguments->FileRead, argv[1]);
    printf("%s\n", arguments->FileRead);                                      

    read_done = write_done = 0;

    mycoroutines_init(&main_ctx);

    if(!read_done || !write_done) {
        res1 = mycoroutines_create(&write_ctx, routine_write, arguments);
        res2 = mycoroutines_create(&read_ctx, routine_read, arguments);

        if((res1 == -1) || (res2 == -1)) 
            return -1;

        mycoroutines_switchto(&main_ctx, &write_ctx);
    }

    printf("\nDone!\n");

    free(arguments->FileRead);
    free(arguments);

    return 0; 
}