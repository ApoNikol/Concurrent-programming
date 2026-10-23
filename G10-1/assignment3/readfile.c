#include <stdio.h>
int main() {
    // Open the file in write mode
    FILE *file = fopen("random_numbers.bin", "rb"); //wb an theleis na ftakseis arxeio rb an thes na diavaseis
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    } 
    // Read and print the values from the file
    int value;
    while (fread(&value, sizeof(int), 1, file)) {
        printf("%d\n", value);
    }

    // Close the file after reading
    fclose(file);
    return 0;
}
