
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
	int pipefd[2];
	pid_t curr_pid;

	if(pipe(pipefd) == -1){

		perror("pipe");
		exit(1);
	}

	curr_pid = fork();
	
	if(curr_pid == -1){
		perror("fork");
		exit(1);	
	}

	if(curr_pid == 0){

		close(pipefd[0]);
		write(pipefd[1], "hello", 5);
		close(pipefd[1]);

		_exit(0);
	}
	else{

		close(pipefd[1]);
		char buf[1];
		while(read(pipefd[0], buf, 1) > 0){

			printf("%c", buf[0]);
		}
		printf("\n");
		
		close(pipefd[0]);
		exit(0);
	}



	return 0;
}