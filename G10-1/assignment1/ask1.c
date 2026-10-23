#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "pipelib.h"

int read_done;
int write_done;

typedef struct {
    int id;
    int filetowrite;
    char *FileRead;
} thread_data;

void *routine_write(void *argv) {
    thread_data data = *(thread_data *)argv;
    FILE *FileName;                      
    char character;
    int res;

    FileName = fopen(data.FileRead, "r+");

    if (FileName == NULL) {
        printf("Invalid file.\n");
        exit(1);
    }
    
    while(fread(&character, 1, sizeof(char), FileName)) {   //fread
        res = pipe_write(data.id, character); 
        if(res == -1)
            return NULL;    //isws return
    }
    
    res = pipe_writeDone(data.id);
    if(res == -1) {
        printf("pipe_writeDone failed.");
        return NULL;       //isws return
    } 

    printf("Closed write\n");
    write_done = 1;
    return NULL;
}

void *routine_read(void *argv) {
    thread_data data = *(thread_data *)argv;
    FILE *FileName;                      
    char character;
    int res;
    int new_length;
    char *new_filename;
    char *dot;
    
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
    
    if(data.filetowrite == 2) {
        data.FileRead = realloc(data.FileRead, sizeof(char) * new_length);
        strcpy(data.FileRead, new_filename);

        dot = strrchr(data.FileRead, '.');
        new_length = (dot - data.FileRead) + strlen(".copy2") + 1;
        new_filename = (char *)malloc(new_length);
        
        if (new_filename == NULL) {
            printf("Memory allocation failed!\n");
            exit(1);
        }
        
        strncpy(new_filename, data.FileRead, dot - data.FileRead);
        new_filename[dot - data.FileRead] = '\0';
        strcat(new_filename, ".copy2");
    }

    FileName = fopen(new_filename, "w+");
    free(new_filename);

    while(1) {
        res = pipe_read(data.id, &character);
        if(res != 1)
            break;         
        res = fwrite(&character, 1, sizeof(char), FileName);           
    }   
    if(res == -1) {
        printf("No pipe with this id.\n");
        fclose(FileName);
        read_done = 1;
        return NULL;    
    }
    else {
        printf("Closed read\n");
        fclose(FileName);
        read_done = 1;
        
        return NULL;
    }

    fclose(FileName);
    return NULL;
}



int main(int argc, char *argv[]) {
    int res1, res2, tmp;
    pthread_t thread1, thread2; 
    thread_data *arguments;

    if(argc != 2) {
        printf("Wrong number of arguments\n");
        return -1;
    }

    arguments = (thread_data *)malloc(sizeof(thread_data));
    arguments->id = pipe_open(64);
    tmp = pipe_open(64);
    printf("First pipe opened at index: %d\n", arguments->id);
    printf("Second pipe opened at index: %d\n", tmp);

    arguments->FileRead = (char *)malloc(sizeof(char) * (strlen(argv[1]) + 1));
    strcpy(arguments->FileRead, argv[1]);                                      

    read_done = write_done = 0;
    arguments->filetowrite = 1;

    res1 = pthread_create(&thread1, NULL, routine_write, arguments);
    res2 = pthread_create(&thread2, NULL, routine_read, arguments);

    if((res1) || (res2)) 
        return -1;

    while(!read_done || !write_done) 
        continue;

    read_done = write_done = 0;
    arguments->filetowrite = 2;
    arguments->id = tmp;
    
    res1 = pthread_create(&thread1, NULL, routine_write, arguments);
    res2 = pthread_create(&thread2, NULL, routine_read, arguments);

    if((res1) || (res2)) 
        return -1;

    while(!read_done || !write_done) 
        continue;
    
    free(arguments->FileRead);
    free(arguments);

    return 0; 
}
