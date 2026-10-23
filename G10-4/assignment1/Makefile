# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Targets
TARGET = ask1
SRC = ask1.c coroutines.c pipelib.c
OBJ = $(SRC:.c=.o)

# Default rule
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean rule
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
