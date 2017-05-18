
#include "LineParser.c"


int main(int argc, char** argv){

  initialize_shell();

  debugger(argc, argv);


  while(1){
    
    printCurrentPath();

    char userInput[2048];

    printf(" >> ");
    fgets(userInput, 2048, stdin);

    if(strcmp(userInput, "quit\n") == 0){
      freeJobList(jobs);
      free(initial_tmodes);
      exit(0);
    }
    
    if(userInput[0] != '\n'){

      cmdLine* cmdLine = parseCmdLines(userInput);
      // printCommandLine(cmdLine);
      execute(cmdLine);
    }
  }

  return 0;
}