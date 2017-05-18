
all: main

main: JobControl.o  main.o
	gcc -g  -Wall JobControl.o  main.o  -o main

main.o: main.c  LineParser.h
	gcc -g  -Wall -c main.c -o main.o

JobControl.o: JobControl.c  JobControl.h
	gcc -g  -Wall -c JobControl.c -o JobControl.o

.PHONY:
	clean

clean:
	rm -f ./*.o  main