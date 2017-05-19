#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


typedef struct variable
{
    char* name;
    char* value;
    struct variable* next; /* next variable in list */
}variable;

variable* addVariable(variable** vars_set, char* name, char* value);
void freeVariableSet(variable** vars_set);
void freeVariable(variable* var_to_remove);
void printVars(variable** vars_set);
void printVar(variable* var);
variable* createdNewVar(char* name, char* value);
char* findVar(variable** vars, char* varName);
int removeVar(variable** vars_set, char* var_to_remove);