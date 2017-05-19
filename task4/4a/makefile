
all: main

main: JobControl.o  LineParser.o variable.o
	gcc -g  -Wall JobControl.o  LineParser.o variable.o -o main

LineParser.o: LineParser.c  LineParser.h
	gcc -g  -Wall -c LineParser.c -o LineParser.o

JobControl.o: JobControl.c  JobControl.h
	gcc -g  -Wall -c JobControl.c -o JobControl.o

variable.o: variable.c variable.h
	gcc -g  -Wall -c variable.c -o variable.o


.PHONY:
	clean

clean:
	rm -f ./*.o  main