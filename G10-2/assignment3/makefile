# Compiler and flags
CC = gcc
CFLAGS = -Wall -pthread

# Sources and headers
MAIN = ask3.c
LIB = semaph_lib.c
HEADERS = semaph_lib.h

# Objects and target
OBJS = ask3.o semaph_lib.o
TARGET = ask3

# Default target
all: $(TARGET)

# Linking the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile main program
ask3.o: $(MAIN) $(HEADERS)
	$(CC) $(CFLAGS) -c $(MAIN)

# Compile semaphore library
semaph_lib.o: $(LIB) $(HEADERS)
	$(CC) $(CFLAGS) -c $(LIB)

# Clean up the build files
clean:
	rm -f $(OBJS) $(TARGET)
