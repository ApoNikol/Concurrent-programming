#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    const int count = 10000;
    int numbers[count];

    // Seed the random number generator with the current time

    // Generate 10000 random long long int values
    for (int i = 0; i < count; i++) {
        // Generate a random long long int value by combining two random ints
        numbers[i] = (int)rand() % 1000;
    }

    // Open the binary file for writing
    FILE *file = fopen("random_numbers.bin", "wb");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Write the array to the file
    size_t written = fwrite(numbers, sizeof(int), count, file);
    if (written != count) {
        perror("Error writing to file");
        fclose(file);
        return EXIT_FAILURE;
    }

    printf("Successfully written %d random int values to 'random_numbers.bin'\n", count);

    // Close the file
    fclose(file);

    return EXIT_SUCCESS;
}
