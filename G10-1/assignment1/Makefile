CC = gcc

CFLAGS = -Wall -pthread

OBJS = ask1.o pipelib.o

TARGET = ask1

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

ask1.o: ask1.c pipelib.h
	$(CC) $(CFLAGS) -c ask1.c

pipelib.o: pipelib.c pipelib.h
	$(CC) $(CFLAGS) -c pipelib.c

clean:
	rm -f $(OBJS) $(TARGET)




