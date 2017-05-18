
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
	int pipefd[2];
	
	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		exit(1);
	}

	
	
	return 0;
}