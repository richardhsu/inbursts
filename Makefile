CC=gcc
CFLAGS=-I.
DEPS = inbursts.h
OBJ = inbursts.o
LIBS = -lpcap

inbursts: inbursts.c
	gcc -o inbursts inbursts.c $(CFLAGS) $(LIBS)

clean:
	rm -f *.o inbursts
