CC=gcc
DEBUG= -g
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -Wall $(DEBUG)
SOURCE= jms_console.c jms_coord.c pool.c
HEADER= pool.h
OBJ= jms_console.o jms_coord.o pool.o
OUT= jms_console jms_coord pool

all: jms_console jms_coord pool

jms_console: jms_console.o
	$(CC) $(LFLAGS) jms_console.o -o jms_console

jms_console.o: jms_console.c pool.h functions.h
	$(CC) $(CFLAGS) jms_console.c

jms_coord: jms_coord.o
	$(CC) $(LFLAGS) jms_coord.o -o jms_coord

jms_coord.o: jms_coord.c functions.h pool.h
	$(CC) $(CFLAGS) jms_coord.c

pool: pool.o
	$(CC) $(LFLAGS) pool.o -o pool

pool.o: pool.c pool.h functions.h
	$(CC) $(CFLAGS) pool.c

clean:
	rm -f $(OBJ) $(OUT)

count:
	wc $(SOURCE) $(HEADER)
