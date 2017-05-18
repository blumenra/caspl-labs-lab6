
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define STDIN 0
#define STDOUT 1

int debug = 0;

void debugger(int argc, char** argv);

int main(int argc, char** argv)
{
	
	debugger(argc, argv);

	int pipefd[2];
	pid_t forked1;
	pid_t forked2;
	int status;

	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		exit(1);
	}

	if(debug)
		fprintf(stderr, "parent_process>forking…\n");
	

	forked1 = fork();

	if(debug)
		fprintf(stderr, "parent_process>created process with id: %d\n", forked1);
	

	if(forked1 == -1)
	{
		perror("fork1");
		exit(1);
	}

	if (forked1 == 0)
	{
		if(debug)
			fprintf(stderr, "child1>redirecting stdout to the write end of the pipe…\n");
		

		close(STDOUT);
		int fg;

		fg = dup(pipefd[1]);

		if (fg == -1)
		{
			perror("dup on fork1");
			exit(1);
		}

		close(pipefd[1]);

		char* const fileArgv[3] = {"ls", "-l"};

		if(debug)
		{
			fprintf(stderr, "child1>going to execute cmd: ");
			int i;
			for(i=0; i < 2; i++)
			{
				fprintf(stderr, "%s ", fileArgv[i]);
			}
			fprintf(stderr, "\n");
		}


		if (execvp(fileArgv[0], fileArgv) == -1)
		{
			perror("execvp");
		}
		
		_exit(1);
	}

	if(debug)
		fprintf(stderr, "parent_process>closing the write end of the pipe…\n");


	close(pipefd[1]);


	if(debug)
		fprintf(stderr, "parent_process>forking…\n");


	forked2 = fork();

	if(debug)
		fprintf(stderr, "parent_process>created process with id: %d\n", forked2);
	

	if(forked2 == -1)
	{
		perror("fork2");
		exit(1);
	}

	if(forked2 == 0)
	{

		if(debug)
			fprintf(stderr, "child2>redirecting stdin to the read end of the pipe…\n");
		

		close(STDIN);
		int fg;

		fg = dup(pipefd[0]);

		if (fg == -1)
		{
			perror("dup on fork1");
			exit(1);
		}

		close(pipefd[0]);

		char* const fileArgv[4] = {"tail", "-n", "2"};

		if(debug)
		{
			fprintf(stderr, "child2>going to execute cmd: ");
			int i;
			for(i=0; i < 3; i++)
			{
				fprintf(stderr, "%s ", fileArgv[i]);
			}
			fprintf(stderr, "\n");
		}

		if (execvp(fileArgv[0], fileArgv) == -1)
		{
			perror("execvp");
		}
		
		_exit(1);
	}

	if(debug)
		fprintf(stderr, "parent_process>closing the read end of the pipe…\n");
	

	close(pipefd[0]);

	if(debug)
		fprintf(stderr, "parent_process>waiting for child processes %d to terminate…\n", forked1);
	

	waitpid(forked1, &status, 0);

	if(debug)
		fprintf(stderr, "parent_process>waiting for child processes %d to terminate…\n", forked2);
	

	waitpid(forked2, &status, 0);


	if(debug)
		fprintf(stderr, "parent_process>exiting…\n");
	

	return 0;
}


void debugger(int argc, char** argv){

  int i;

  for(i = 1; i < argc; i++){

    char* currArg;
    currArg = argv[i];

    if((strcmp(currArg, "-d") == 0)){
      debug = i;
    }
  }
}