CC = gcc

CFLAGS = -Wall -pedantic

all: main triage reparto prestazione 

main: main.o my_functions.o
	$(CC) $(CFLAGS) -o main main.o my_functions.o

main.o: main.c my_types.h
	$(CC) $(CFLAGS) -c main.c

triage: triage.o my_functions.o
	$(CC) $(CFLAGS) -o triage triage.o my_functions.o

triage.o: triage.c my_types.h
	$(CC) $(CFLAGS) -c triage.c

reparto: reparto.o my_functions.o
	$(CC) $(CFLAGS) -o reparto reparto.o my_functions.o

reparto.o: reparto.c my_types.h
	$(CC) $(CFLAGS) -c reparto.c

prestazione: prestazione.o my_functions.o
	$(CC) $(CFLAGS) -o prestazione prestazione.o my_functions.o

prestazione.o: prestazione.c my_types.h
	$(CC) $(CFLAGS) -c prestazione.c
	
my_functions.o: my_functions.c my_types.h
	$(CC) $(CFLAGS) -c my_functions.c

clean:
	$(RM) main triage reparto prestazione *.o *.~