#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "LineParser.h"
#include "JobControl.h"
#include "variable.h"
#include <linux/limits.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1

#ifndef NULL
    #define NULL 0
#endif

#define FREE(X) if(X) free((void*)X)

int printCommandLine(cmdLine* cmdLine);
void debugger(int argc, char** argv);
int printCurrentPath();
int execute(cmdLine *pCmdLine);
void reactToSignal(int signal);
void setupSignals(int parent);
void setupSignal(int sig, int dfl);
void handleNewJob(job** Job_list, cmdLine* cmd);
void handleNewPipedJob(job** Job_list, cmdLine* pCmdLine);
int specialCommand(cmdLine *pCmdLine);
int delay();
void initialize_shell();
void set_pgid(pid_t pid, pid_t pgid);
int print_tmodes(struct termios *tmodes);
void assignEnvVars(cmdLine* pCmdLine);





int debug = 0;
job* jobs[] = {0};
variable* vars[] = {0};
struct termios *initial_tmodes = NULL;
pid_t shell_pgid;



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
      freeVariableSet(vars);
      free(initial_tmodes);
      exit(0);
    }
    
    if(userInput[0] != '\n'){

      cmdLine* cmdLine = parseCmdLines(userInput);
      execute(cmdLine);
    }
  }

  return 0;
}


int execute(cmdLine *pCmdLine){

  assignEnvVars(pCmdLine);

  int piped = (pCmdLine->next != NULL);

  if(!specialCommand(pCmdLine)){

    if(piped)
      handleNewPipedJob(jobs, pCmdLine);
    else
      handleNewJob(jobs, pCmdLine);
  }

  freeCmdLines(pCmdLine);

  return 0;
}


void assignEnvVars(cmdLine* pCmdLine){

  cmdLine* tmpCmd = pCmdLine;

  while(tmpCmd != NULL){

    int i;
    for(i=1; i < tmpCmd->argCount; i++){

      char* arg = tmpCmd->arguments[i];
      char firstChar = arg[0];
      if(firstChar == '$'){

        char varName[MAX_ARGUMENTS];
        strcpy(varName, arg+1);
        char* ret = findVar(vars, varName);

        if(ret != NULL){
          
          replaceCmdArg(tmpCmd, i, ret);

          free(ret);
        }
        else{
          fprintf(stderr, "The variable %s was not defined\n", arg+1);
        }
      }
    }

    tmpCmd = tmpCmd->next;
  }
}

void initialize_shell(){

  initial_tmodes = (struct termios*) (malloc(sizeof(struct termios)));
  setupSignals(1);
  set_pgid(getpid(), getpid());

  tcsetpgrp(STDIN_FILENO, getpid());

  if(tcgetattr(STDIN_FILENO, initial_tmodes) == -1){

    perror("tcgetattr");
    exit(0);
  }

  shell_pgid = getpgid(getpid());
}


void set_pgid(pid_t pid, pid_t pgid){

  if(setpgid(pid, pgid) == -1){

    perror("setpgid");
    exit(0);
  }
}

void handleNewPipedJob(job** Job_list, cmdLine* pCmdLine){

  cmdLine* tmpCmd;

  char fullCmd[MAX_ARGUMENTS];
  tmpCmd = pCmdLine;
  strcpy(fullCmd, tmpCmd->arguments[0]);
  int i;
  for(i=1; i < tmpCmd->argCount; i++){
    strcat(fullCmd, " ");
    strcat(fullCmd, tmpCmd->arguments[i]);
  }

  int pipefd[2];
  pid_t forked1;
  pid_t forked2;

  job* j1 = addJob(Job_list, fullCmd);


  char fullCmd2[MAX_ARGUMENTS];
  tmpCmd = pCmdLine->next;
  strcpy(fullCmd2, tmpCmd->arguments[0]);
  for(i=1; i < tmpCmd->argCount; i++){
    strcat(fullCmd2, " ");
    strcat(fullCmd2, tmpCmd->arguments[i]);
  }


  job* j2 = addJob(Job_list, fullCmd2);
  j1->status = RUNNING;
  j2->status = RUNNING;

  if (pipe(pipefd) == -1)
  {
    perror("pipe");
    exit(1);
  }

  forked1 = fork();
  j1->pgid = forked1;
  set_pgid(forked1, forked1);


  if(forked1 == -1){
    perror("fork1");
    exit(1);
  }

  if (forked1 == 0){
    
    if(debug){
      fprintf(stderr, "Child PID is %ld\n", (long) getpid());
      fprintf(stderr, "Child PGID is %ld\n", (long) getpgrp());
      fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
    }

    int fd;

    if(pCmdLine->inputRedirect != NULL){
      
      close(STDIN);
      fd = open(pCmdLine->inputRedirect, O_RDWR);

      if(fd == 0){

        dup(fd);
      }
    }
    if(pCmdLine->outputRedirect != NULL){

      close(STDOUT);
      fd = open(pCmdLine->outputRedirect, O_RDWR);
      if(fd == 1){

        dup(fd);
      }
    }

    setupSignals(0);

    close(STDOUT);
    int fg;

    fg = dup(pipefd[1]);

    if (fg == -1){
      perror("dup on fork1");
      exit(1);
    }

    close(pipefd[1]);

    if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1){
      perror("execvp");
    }
    
    _exit(1);
  }

  close(pipefd[1]);

  forked2 = fork();
  j2->pgid = forked1;
  set_pgid(forked2, forked1);


  if(forked2 == -1){
    perror("fork2");
    exit(1);
  }

  if(forked2 == 0){
    
    if(debug){
      fprintf(stderr, "Child PID is %ld\n", (long) getpid());
      fprintf(stderr, "Child PGID is %ld\n", (long) getpgrp());
      fprintf(stderr, "Executing command: %s\n", pCmdLine->next->arguments[0]);
    }

    int fd;

    if(pCmdLine->next->inputRedirect != NULL){
      

      close(STDIN);
      fd = open(pCmdLine->next->inputRedirect, O_RDWR);

      if(fd == 0){

        dup(fd);
      }
    }
    if(pCmdLine->next->outputRedirect != NULL){

      close(STDOUT);
      fd = open(pCmdLine->next->outputRedirect, O_RDWR);
      if(fd == 1){
        dup(fd);
      }
    }

    setupSignals(0);
    
    close(STDIN);
    int fg;

    fg = dup(pipefd[0]);

    if (fg == -1){
      perror("dup on fork1");
      exit(1);
    }

    close(pipefd[0]);

    sleep(1);

    if (execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments) == -1){
      perror("execvp");
    }
    
    _exit(1);
  }

  close(pipefd[0]);

  delay();

  runJobInForeground (jobs, j1, 0, initial_tmodes, shell_pgid);
  runJobInBackground(j2, 0);
}


void handleNewJob(job** Job_list, cmdLine* pCmdLine){

  char fullCmd[MAX_ARGUMENTS];
  strcpy(fullCmd, pCmdLine->arguments[0]);
  int i;
  for(i=1; i < pCmdLine->argCount; i++){
    strcat(fullCmd, " ");
    strcat(fullCmd, pCmdLine->arguments[i]);
  }

  pid_t curr_pid;


  job* newJob = addJob(Job_list, fullCmd);
  newJob->status = RUNNING;

  curr_pid = fork();
  newJob->pgid = curr_pid;
  set_pgid(curr_pid, curr_pid);


  if(curr_pid == -1){

    perror("fork");
    exit(EXIT_FAILURE);
  }
  
  if(curr_pid == 0){
    
    if(debug){
      fprintf(stderr, "Child PID is %ld\n", (long) getpid());
      fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
    }

    int fd;

    if(pCmdLine->inputRedirect != NULL){
      
      close(STDIN);
      fd = open(pCmdLine->inputRedirect, O_RDWR);

      if(fd == 0){

        dup(fd);
      }
    }
    if(pCmdLine->outputRedirect != NULL){

      close(STDOUT);
      fd = open(pCmdLine->outputRedirect, O_RDWR);
      if(fd == 1){

        dup(fd);
      }
    }

    setupSignals(0);

    newJob->pgid = getgid();

    if(execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1){
      
      perror("execv");
      _exit(EXIT_FAILURE);
    }

    exit(1);
  }
  

  delay();

  // if it's a blocking command' wait for the child process (0) to end before proceeding
  if(pCmdLine->blocking){

    runJobInForeground (jobs, newJob, 0, initial_tmodes, shell_pgid);
  }
  else{

    runJobInBackground(newJob, 0);
  }
}


int delay(){

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 5000000L;

  return nanosleep(&tim, &tim2);
}


int specialCommand(cmdLine *pCmdLine){

  int special = 0;
  char* command = pCmdLine->arguments[0];

  if(strcmp(command, "cd") == 0){

    special = 1;
    if(strcmp(pCmdLine->arguments[1], "~") == 0){
      replaceCmdArg(pCmdLine, 1, "/");
    }
    
    if(chdir(pCmdLine->arguments[1]) == -1){
      
      perror("cd");
    }
  }
  else if(strcmp(command, "jobs") == 0){

    special = 1;

    printJobs(jobs);
  }
  else if(strcmp(command, "fg") == 0){

    if(pCmdLine->argCount < 2){

      fprintf(stderr, "Invalid input of fg. Index is missing.\n");
    }
    else{

      job* tmp = findJobByIndex(*jobs, atoi(pCmdLine->arguments[1]));
      if(tmp != NULL){

        runJobInForeground (jobs, tmp, 1, initial_tmodes, shell_pgid);
      }
    }
      special = 1;
  }
  else if(strcmp(command, "bg") == 0){

    if(pCmdLine->argCount < 2){

      fprintf(stderr, "Invalid input of bg. Index is missing.\n");
    }
    else{

      job* tmp = findJobByIndex(*jobs, atoi(pCmdLine->arguments[1]));
      if(tmp != NULL){

        runJobInBackground(tmp, 1);
      }
    }
    
    special = 1;
  }
  else if(strcmp(command, "set") == 0){
    if(pCmdLine->argCount != 3){
      
      fprintf(stderr, "Invalid amount of arguments for set. Please enter 'set <arg1> <arg2>'.\n");
    }
    else{
      addVariable(vars, pCmdLine->arguments[1], pCmdLine->arguments[2]);
    }
    
    special = 1;
  }
  else if(strcmp(command, "env") == 0){
    printVars(vars);

    special = 1;
  }
  else if(strcmp(command, "delete") == 0){
    if(pCmdLine->argCount != 2){
      
      fprintf(stderr, "Invalid amount of arguments for delete. Please enter 'delete <env_variable>'.\n");
    }
    else{
      if(removeVar(vars, pCmdLine->arguments[1])){
        printf("%s was deleted.\n", pCmdLine->arguments[1]);
      }
      else{
        
        fprintf(stderr, "The variable %s is not defined.\n", pCmdLine->arguments[1]);
      }
    }
    
    special = 1;
  }


  return special;
}


int print_tmodes(struct termios *tmodes){

  if(tmodes == NULL){

    printf("The termios is NULL!\n");
    return -1;
  }

  printf("c_iflag: %d\n", (int) tmodes->c_iflag);
  printf("c_oflag: %d\n", (int) tmodes->c_oflag);
  printf("c_cflag: %d\n", (int) tmodes->c_cflag);
  printf("c_lflag: %d\n", (int) tmodes->c_lflag);
  
  printf("c_cc: ");
  int i;
  int len = (int) strlen((char*) tmodes->c_cc);
  for(i=0; i < len; i++){

    printf("%d", (int) tmodes->c_cc[i]);
  }
  printf("\n");

  return 0;
}


int printCommandLine(cmdLine* cmdLine){

  if(cmdLine == NULL){
    return 0;
  }

  int i;
  
  printf("arguments: ");
  for(i = 0; i < cmdLine->argCount; i++){

    printf("%s ", cmdLine->arguments[i]);
  }
  puts("");

  printf("argCount: %d\n", cmdLine->argCount);
  printf("inputRedirect: %s\n", cmdLine->inputRedirect);
  printf("outputRedirect: %s\n", cmdLine->outputRedirect);
  printf("blocking: %c\n", cmdLine->blocking);
  printf("idx: %d\n", cmdLine->idx);
  printCommandLine(cmdLine->next);

  return 0;
}


int printCurrentPath(){
  
  char buf[PATH_MAX];

  getcwd(buf, PATH_MAX);

  printf("%s", buf);  
  
  return 0;  
}


// Ignore the following signals: SIGTTIN, SIGTTOU and use your signal handler from task0b
// to handle the following signals: SIGQUIT, SIGCHLD, SIGSTP
// (We're not including SIGINT here so you can kill the shell with ^C if there's a bug somewhere)
void setupSignals(int parent){

  setupSignal(SIGTTIN, !parent);
  setupSignal(SIGTTOU, !parent);
  setupSignal(SIGQUIT, !parent);
  setupSignal(SIGTSTP, !parent);
  setupSignal(SIGCHLD, !parent);
}


void setupSignal(int sig, int dfl){

  if(!dfl){

    if((sig == SIGTTIN) || (sig == SIGTTOU) || (sig == SIGTSTP)){

      if(signal(sig, SIG_IGN) == SIG_ERR){

        perror("signal");
        exit(EXIT_FAILURE); 
      }
    }
    else{
     
      if(signal(sig, reactToSignal) == SIG_ERR){

        perror("signal");
        exit(EXIT_FAILURE); 
      }
    }
  }
  else{

    if(signal(sig, SIG_DFL) == SIG_ERR){

      perror("signal");
      exit(EXIT_FAILURE); 
    }
  }
}


void reactToSignal(int signal){

  // printf("\nThe signal that was recieved is '%s'\n", strsignal(signal));
  // printf("The signal was ignored.\n");
  // printf("\n");
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


static char *cloneFirstWord(char *str)
{
    char *start = NULL;
    char *end = NULL;
    char *word;

    while (!end) {
        switch (*str) {
            case '>':
            case '<':
            case 0:
                end = str - 1;
                break;
            case ' ':
                if (start)
                    end = str - 1;
                break;
            default:
                if (!start)
                    start = str;
                break;
        }
        str++;
    }

    if (start == NULL)
        return NULL;

    word = (char*) malloc(end-start+2);
    strncpy(word, start, ((int)(end-start)+1)) ;
    word[ (int)((end-start)+1)] = 0;

    return word;
}

static void extractRedirections(char *strLine, cmdLine *pCmdLine)
{
    char *s = strLine;

    while ( (s = strpbrk(s,"<>")) ) {
        if (*s == '<') {
            FREE(pCmdLine->inputRedirect);
            pCmdLine->inputRedirect = cloneFirstWord(s+1);
        }
        else {
            FREE(pCmdLine->outputRedirect);
            pCmdLine->outputRedirect = cloneFirstWord(s+1);
        }

        *s++ = 0;
    }
}

static char *strClone(const char *source)
{
    char* clone = (char*)malloc(strlen(source) + 1);
    strcpy(clone, source);
    return clone;
}

static int isEmpty(const char *str)
{
  if (!str)
    return 1;
  
  while (*str)
    if (!isspace(*(str++)))
      return 0;
    
  return 1;
}

static cmdLine *parseSingleCmdLine(const char *strLine)
{
    char *delimiter = " ";
    char *line, *result;
    
    if (isEmpty(strLine))
      return NULL;
    
    cmdLine* pCmdLine = (cmdLine*)malloc( sizeof(cmdLine) ) ;
    memset(pCmdLine, 0, sizeof(cmdLine));
    
    line = strClone(strLine);
         
    extractRedirections(line, pCmdLine);
    
    result = strtok( line, delimiter);    
    while( result && pCmdLine->argCount < MAX_ARGUMENTS-1) {
        ((char**)pCmdLine->arguments)[pCmdLine->argCount++] = strClone(result);
        result = strtok ( NULL, delimiter);
    }

    FREE(line);
    return pCmdLine;
}

static cmdLine* _parseCmdLines(char *line)
{
  char *nextStrCmd;
  cmdLine *pCmdLine;
  char pipeDelimiter = '|';
  
  if (isEmpty(line))
    return NULL;
  
  nextStrCmd = strchr(line , pipeDelimiter);
  if (nextStrCmd)
    *nextStrCmd = 0;
  
  pCmdLine = parseSingleCmdLine(line);
  if (!pCmdLine)
    return NULL;
  
  if (nextStrCmd)
    pCmdLine->next = _parseCmdLines(nextStrCmd+1);

  return pCmdLine;
}

cmdLine *parseCmdLines(const char *strLine)
{
  char* line, *ampersand;
  cmdLine *head, *last;
  int idx = 0;
  
  if (isEmpty(strLine))
    return NULL;
  
  line = strClone(strLine);
  if (line[strlen(line)-1] == '\n')
    line[strlen(line)-1] = 0;
  
  ampersand = strchr( line,  '&');
  if (ampersand)
    *(ampersand) = 0;
    
  if ( (last = head = _parseCmdLines(line)) )
  { 
    while (last->next)
      last = last->next;
    last->blocking = ampersand? 0:1;
  }
  
  for (last = head; last; last = last->next)
    last->idx = idx++;
      
  FREE(line);
  return head;
}


void freeCmdLines(cmdLine *pCmdLine)
{
  int i;
  if (!pCmdLine)
    return;

  FREE(pCmdLine->inputRedirect);
  FREE(pCmdLine->outputRedirect);
  for (i=0; i<pCmdLine->argCount; ++i)
      FREE(pCmdLine->arguments[i]);

  if (pCmdLine->next)
    freeCmdLines(pCmdLine->next);

  FREE(pCmdLine);
}

int replaceCmdArg(cmdLine *pCmdLine, int num, const char *newString)
{
  if (num >= pCmdLine->argCount)
    return 0;
  
  FREE(pCmdLine->arguments[num]);
  ((char**)pCmdLine->arguments)[num] = strClone(newString);
  return 1;
}
