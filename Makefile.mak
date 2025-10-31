CC = gcc
CFLAGS = -Wall -pthread -O2
DEPS = hash_utils.h
OBJ = hash_utils.o

all: servidor worker

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

servidor: servidor.c $(OBJ)
	$(CC) -o servidor servidor.c $(OBJ) $(CFLAGS)

worker: worker.c $(OBJ)
	$(CC) -o worker worker.c $(OBJ) $(CFLAGS)

clean:
	rm -f *.o servidor worker

.PHONY: all clean