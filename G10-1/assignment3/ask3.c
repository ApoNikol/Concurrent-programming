#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

char *FileName;

typedef struct {
    int start;
    int end;
    int thread_done;  
} thread_data;

void insertionSortFile(FILE *file, int start, int end) {
    int key, j, value;

    //printf("start: %d end: %d\n", start, end);
    for (int i = start; i < end; i++) {
        fseek(file, i * sizeof(int), SEEK_SET);
        fread(&key, sizeof(int), 1, file);

        j = i - 1;

        while (j >= start) {                          
            fseek(file, j * sizeof(int), SEEK_SET);
            fread(&value, sizeof(int), 1, file);

            if (value > key) {
                fseek(file, (j + 1) * sizeof(int), SEEK_SET);
                fwrite(&value, sizeof(int), 1, file);
                j--;
            } 
            else 
                break;
        }

        fseek(file, (j + 1) * sizeof(int), SEEK_SET);
        fwrite(&key, sizeof(int), 1, file);
    }
}

void merge_sections(FILE *file, int start, int end) {
    int i = start, j = (start+end) / 2;
    int left_value, right_value;

    while (i < j && j < end) {
        fseek(file, i * sizeof(int), SEEK_SET);
        fread(&left_value, sizeof(int), 1, file);

        fseek(file, j * sizeof(int), SEEK_SET);
        fread(&right_value, sizeof(int), 1, file);

        if (left_value <= right_value) {
            i++;
        } 
        else {
            for (int k = j; k > i; k--) {
                fseek(file, (k - 1) * sizeof(int), SEEK_SET);
                fread(&left_value, sizeof(int), 1, file);  

                fseek(file, k * sizeof(int), SEEK_SET);
                fwrite(&left_value, sizeof(int), 1, file); 
            }
            fseek(file, i * sizeof(int), SEEK_SET);
            fwrite(&right_value, sizeof(int), 1, file);
            i++;
            j++;
        }
    }
}

void *routine(void *argv) {
    pthread_t thread1, thread2;
    thread_data *main_thread_data, *thread_data1, *thread_data2;
    int length, num_elements;
    int res1, res2, start, end;
    
    main_thread_data = (thread_data *)argv;
    length = main_thread_data->end - main_thread_data->start;
    num_elements = length / sizeof(int);

    printf("ELEMENTS: %d", num_elements);
    FILE *file = fopen(FileName, "rb+");
    if (file == NULL) {
        perror("Failed to open file.\n");
        return NULL;
    }

    start = main_thread_data->start / sizeof(int);
    end = main_thread_data->end / sizeof(int);
    
    if(num_elements <= 64) {
        insertionSortFile(file, start, end);
        fclose(file);                              
        (main_thread_data->thread_done) = 1;

        return NULL;
    }

    thread_data1 = (thread_data *)malloc(sizeof(thread_data));
    thread_data2 = (thread_data *)malloc(sizeof(thread_data));
 
    thread_data1->start = main_thread_data->start;
    thread_data2->end = main_thread_data->end;

    if (num_elements % 2 == 0) {
        thread_data1->end = thread_data1->start + (length / 2);                      
        thread_data2->start = thread_data1->end;    
    }
    else {
        thread_data1->end = thread_data1->start + (length / 2) - (sizeof(int) / 2);    
        thread_data2->start = thread_data1->end;
    }
    (thread_data1->thread_done) = (thread_data2->thread_done) = 0;

    res1 = pthread_create(&thread1, NULL, routine, thread_data1);
    res2 = pthread_create(&thread2, NULL, routine, thread_data2);

    if((res1) || (res2)) {
        perror("Error with thread creation.\n");
        return NULL;
    }

    while(!(thread_data1->thread_done) || !(thread_data2->thread_done))
        continue;
    free(thread_data1);
    free(thread_data2);

    merge_sections(file, start, end);  

    fclose(file);
    (main_thread_data->thread_done) = 1;

    return NULL;
}

int main(int argc, char *argv[]) {

    pthread_t main_thread; 
    int length, res;
    thread_data *main_thread_data;
    FILE *file;

    if(argc != 2) {
        printf("Wrong number of arguments.\n");
        return -1;
    }

    main_thread_data = (thread_data *)malloc(sizeof(thread_data));
    FileName = (char *)malloc(sizeof(char) * (strlen(argv[1]) + 1));
    strcpy(FileName, argv[1]);                                                                                     
    file = fopen(FileName, "rb+");
    if (file == NULL){
        perror("Failed to open file.\n");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    main_thread_data->start = 0;
    main_thread_data->end = length;
    main_thread_data->thread_done = 0;
    
    res = pthread_create(&main_thread, NULL, routine, main_thread_data);
    if(res) {
        perror("Error with thread creation.\n");
        return -1;
    }

    while(!(main_thread_data->thread_done))
        continue;

    free(main_thread_data);
    free(FileName);
    printf("Sorting Completed.\n");

    return 0;
}
