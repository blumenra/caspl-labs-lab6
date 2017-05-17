
all: mypipe

mypipe: mypipe.o
	gcc -g -m32 -Wall mypipe.o -o mypipe

mypipe.o: mypipe.c
	gcc -g -m32 -Wall -c mypipe.c -o mypipe.o

.PHONY:
	clean

clean:
	rm -f ./*.o mypipe